#include "core.hpp"

Core::Core() :
	texture{2048, 2048, QImage::Format_RGB888} {
}

Core::~Core() {
	releaseEmbree();
}

void Core::loadLowObj(std::string filename) {
	std::vector<tinyobj::shape_t> shapes;
	loadObj(filename, low_attrib, shapes);
	low_shape = shapes[0];
}

void Core::loadHighObj(std::string filename) {
	std::vector<tinyobj::shape_t> shapes;
	loadObj(filename, hi_attrib, shapes);
	hi_shape = shapes[0];

	setupEmbree();
}

void Core::loadObj(	std::string						inputfile, 
					tinyobj::attrib_t&				attrib,
					std::vector<tinyobj::shape_t>&	shapes) {

	std::vector<tinyobj::material_t> materials;

	std::string err;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, inputfile.c_str());

	if(!ret)
		std::cerr << err << std::endl;

	// Verbose info printing
	if(VERBOSE) {
		// Print vertices
		std::cout << attrib.vertices.size() << std::endl;
		std::cout << attrib.normals.size() << std::endl;
		std::cout << attrib.texcoords.size() << std::endl;
		std::cout << shapes[0].mesh.indices.size() << std::endl;

		int j = 0;
		for(int i = 0; i < shapes[0].mesh.indices.size(); ++i) {
			if(shapes[0].mesh.indices[i].vertex_index == 0) {
				++j;
			}
		}
		std::cout << j << std::endl;
	}
}

void Core::setupEmbree() {
	// Activation of "Flush to Zero" and "Denormals are Zero" CPU modes.
	// Embree reccomends them in sake of performance.
	// The #ifndef is needed to make VSCode Intellisense ignore these lines. 
	// It believes that _MM_SET_DENORMALS_ZERO_MODE and _MM_DENORMALS_ZERO_ON are defined nowhere.
	#ifndef __INTELLISENSE__
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	#endif // __INTELLISENSE__

	hi_embree_device = rtcNewDevice(VERBOSE ? "verbose=3" : "verbose=1");
	hi_embree_scene = rtcNewScene(hi_embree_device);

	int noIndices = hi_shape.mesh.indices.size();
	std::vector<uint> triangles(noIndices);
	for(int i = 0; i < noIndices; ++i)
		triangles[i] = (uint)(hi_shape.mesh.indices[i].vertex_index);

	std::vector<float>&	vertices = hi_attrib.vertices;
	const auto geom = rtcNewGeometry(hi_embree_device, RTC_GEOMETRY_TYPE_TRIANGLE);
	rtcSetSharedGeometryBuffer(	geom, RTC_BUFFER_TYPE_VERTEX, 0, 
								RTC_FORMAT_FLOAT3, vertices.data(),
								0, 3*sizeof(float), vertices.size() / 3);
	rtcSetSharedGeometryBuffer(	geom, RTC_BUFFER_TYPE_INDEX, 0, 
								RTC_FORMAT_UINT3, triangles.data(),
								0, 3*sizeof(uint), triangles.size() / 3);
	rtcCommitGeometry(geom);
	const auto geom_id = rtcAttachGeometry(hi_embree_scene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(hi_embree_scene);

}

void Core::bbox(const std::vector<float>& vertices, Vec3f& min, Vec3f& max) {
	const std::vector<float>& vs = vertices; 
	const uint nverts = vs.size() / 3;

	Vec3f p{vs[0], vs[1], vs[2]}; 
	min = p;
	max = p;

	for(int vi = 1; vi < nverts; ++vi) {
		p = {
			vs[3*vi + 0],
			vs[3*vi + 1],
			vs[3*vi + 2]
		};
		if(p[0] < min[0]) min[0] = p[0];
		if(p[1] < min[1]) min[1] = p[1];
		if(p[2] < min[2]) min[2] = p[2];
		if(p[0] > max[0]) max[0] = p[0];
		if(p[1] > max[1]) max[1] = p[1];
		if(p[2] > max[2]) max[2] = p[2];
	}

	if(VERBOSE) {
		std::cout << "bbox: " << min << ", " << max << std::endl;
	}
}

void Core::raycast() {

	const auto topology = hi_shape.mesh;
	const auto geometry = hi_attrib;

	Vec3f bbox_min, bbox_max;
	bbox(hi_attrib.vertices, bbox_min, bbox_max);
	const float scene_dx = bbox_max[0] - bbox_min[0];
	const float scene_dy = bbox_max[1] - bbox_min[1];
	const float scene_dz = bbox_max[2] - bbox_min[2];

	// This data structure is needed by embree. It is never read in this code.
	RTCIntersectContext context;
	rtcInitIntersectContext(&context);

	RTCRayHit rayhit;

	// Orthographic camera on the z axis
	rayhit.ray.dir_x = 0;
	rayhit.ray.dir_y = 0;
	rayhit.ray.dir_z = -1;
	rayhit.ray.org_z = bbox_max[2];
	
	const uint& tex_w = texture.width();
	const uint& tex_h = texture.height();
	uchar* tex_bits = texture.bits();

	for(uint i = 0; i < tex_w; ++i) {
		for(uint j = 0; j < tex_h; ++j) {

			// Map pixel into bbox XY surface
			rayhit.ray.org_x = scene_dx * ((float)i / tex_w) + bbox_min[0];
			rayhit.ray.org_y = scene_dy * ((float)j / tex_h) + bbox_min[1];

			//std::cout << rayhit.ray.org_x << " " << rayhit.ray.org_y << std::endl;

			rayhit.ray.flags = 0;
			rayhit.ray.tnear = 0;
			rayhit.ray.tfar = scene_dz;

			rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

			rtcIntersect1(hi_embree_scene, &context, &rayhit);

			tex_bits[3*(i + j*tex_w) + 0] = 0;
			tex_bits[3*(i + j*tex_w) + 1] = 0;
			tex_bits[3*(i + j*tex_w) + 2] = 0;

			if(rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
				const uint id = rayhit.hit.primID;
				const float a1 = rayhit.hit.u;
				const float a2 = rayhit.hit.v;
				const float a0 = 1 - a1 - a2;

				const int n0_idx = topology.indices[id*3 + 0].normal_index;
				const int n1_idx = topology.indices[id*3 + 1].normal_index;
				const int n2_idx = topology.indices[id*3 + 2].normal_index;
				const Vec3f n0{	geometry.normals[n0_idx*3 + 0],
								geometry.normals[n0_idx*3 + 1],
								geometry.normals[n0_idx*3 + 2]};
				const Vec3f n1{	geometry.normals[n1_idx*3 + 0],
								geometry.normals[n1_idx*3 + 1],
								geometry.normals[n1_idx*3 + 2]};
				const Vec3f n2{	geometry.normals[n2_idx*3 + 0],
								geometry.normals[n2_idx*3 + 1],
								geometry.normals[n2_idx*3 + 2]};

				const Vec3f n = a0*n0 + a1*n1 + a2*n2;

				tex_bits[3*(i + j*tex_w) + 0] = 255 * 0.5 * (n[0] + 1);
				tex_bits[3*(i + j*tex_w) + 1] = 255 * 0.5 * (n[1] + 1);
				tex_bits[3*(i + j*tex_w) + 2] = 255 * 0.5 * (n[2] + 1);
			};
		}
	}
}

void Core::releaseEmbree() {
	rtcReleaseScene(hi_embree_scene);
	rtcReleaseDevice(hi_embree_device);
}