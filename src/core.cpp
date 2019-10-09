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

	rtcInitIntersectContext(&hi_embree_context);
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

void Core::generateNormalMap() {
	
	const auto w = texture.width();
	const auto h = texture.height();
	uchar* tex = texture.bits();

	// Since every pixel is written multiple and variable times we keep the occurence
	std::vector<int> pix_count(w*h, 0);

	std::vector<float> tmp_tex(w*h*3, 0);

	// For each triangle
	const auto trinum = low_shape.mesh.indices.size() / 3;
	for (int ti = 0; ti < trinum; ++ti) {
		const Triangle t = Triangle::fromIndex(ti, low_shape, low_attrib);
		
		const Vec2i uv0i{w * t.uv0[0], h * t.uv0[1]};
		const Vec2i uv1i{w * t.uv1[0], h * t.uv1[1]};
		const Vec2i uv2i{w * t.uv2[0], h * t.uv2[1]};

		const Vec2f v01 = t.uv1 - t.uv0;
		const Vec2f v02 = t.uv2 - t.uv0;
		// Compute matrix
		const Mat2 mat = inv({	v01[0], v02[0],
								v01[1], v02[1]});
		
		Vec2i min, max;

		min = uv0i;
		if(uv1i[0] < min[0]) min[0] = uv1i[0];
		if(uv1i[1] < min[1]) min[1] = uv1i[1];
		if(uv2i[0] < min[0]) min[0] = uv2i[0];
		if(uv2i[1] < min[1]) min[1] = uv2i[1];

		max = uv0i;
		if(uv1i[0] > max[0]) max[0] = uv1i[0];
		if(uv1i[1] > max[1]) max[1] = uv1i[1];
		if(uv2i[0] > max[0]) max[0] = uv2i[0];
		if(uv2i[1] > max[1]) max[1] = uv2i[1];

		// Iterate over texels
		for(int j = min[1]; j <= max[1]; ++j) {
			for(int i = min[0]; i <= max[0]; ++i) {

				for(int us = -1; us <= 1; ++us) {
					for(int vs = -1; vs <= 1; ++vs) {
						// Check if current point is inside
						const Vec2f uv = {	(i + ((float)us / 4)) / w,
											(j + ((float)vs / 4)) / h};
						const Vec2f uvt = mat * (uv - t.uv0);

						const float ct = 1 - uvt[0] - uvt[1];
						const bool inside = uvt[0]	>= 0 && uvt[0]	< 1 &&
											uvt[1]	>= 0 && uvt[1]	< 1 &&
											ct		>= 0 && ct		< 1;

						if(inside) {
							
							const Vec3f pos = ct*t.p0 + uvt[0]*t.p1 + uvt[1]*t.p2;
							const Vec3f dir = ct*t.n0 + uvt[0]*t.n1 + uvt[1]*t.n2;

							const Vec3f n = shootRay(pos, dir);

							const bool noInters = (n[0] + n[1] + n[2]) == 0;

							if(!noInters) {

								// Color texture
								tmp_tex[3*(i + j*w) + 0] += .5 * n[0] + .5;
								tmp_tex[3*(i + j*w) + 1] += .5 * n[1] + .5;
								tmp_tex[3*(i + j*w) + 2] += .5 * n[2] + .5;

								pix_count[i + j*w] += 1;
							}

						}
					}
				}
			}
		}
	}

	for(int j = 0; j < h; ++j) {
		for(int i = 0; i < w; ++i) {
			const int count = pix_count[i + j*w];
			if(count > 0) {
				tex[3*(i + j*w) + 0] = 255 * (tmp_tex[3*(i + j*w) + 0] / count);
				tex[3*(i + j*w) + 1] = 255 * (tmp_tex[3*(i + j*w) + 1] / count);
				tex[3*(i + j*w) + 2] = 255 * (tmp_tex[3*(i + j*w) + 2] / count);
			}
		}
	}
	
}

Triangle Triangle::fromIndex(	const int ti,
								const tinyobj::shape_t& shape,
								const tinyobj::attrib_t& att) {

	const int vidx0 =	shape.mesh.indices[ti*3 + 0].vertex_index;
	const int vidx1 =	shape.mesh.indices[ti*3 + 1].vertex_index;
	const int vidx2 =	shape.mesh.indices[ti*3 + 2].vertex_index;
	const int nidx0 =	shape.mesh.indices[ti*3 + 0].normal_index;
	const int nidx1 =	shape.mesh.indices[ti*3 + 1].normal_index;
	const int nidx2 =	shape.mesh.indices[ti*3 + 2].normal_index;
	const int idx0 =	shape.mesh.indices[ti*3 + 0].texcoord_index;
	const int idx1 =	shape.mesh.indices[ti*3 + 1].texcoord_index;
	const int idx2 =	shape.mesh.indices[ti*3 + 2].texcoord_index;
	return {
		{att.vertices[3*vidx0 + 0],
		 att.vertices[3*vidx0 + 1],
		 att.vertices[3*vidx0 + 2]},
		{att.vertices[3*vidx1 + 0],
		 att.vertices[3*vidx1 + 1],
		 att.vertices[3*vidx1 + 2]},
		{att.vertices[3*vidx2 + 0],
		 att.vertices[3*vidx2 + 1],
		 att.vertices[3*vidx2 + 2]},
		{att.normals[3*nidx0 + 0],
		 att.normals[3*nidx0 + 1],
		 att.normals[3*nidx0 + 2]},
		{att.normals[3*nidx1 + 0],
		 att.normals[3*nidx1 + 1],
		 att.normals[3*nidx1 + 2]},
		{att.normals[3*nidx2 + 0],
		 att.normals[3*nidx2 + 1],
		 att.normals[3*nidx2 + 2]},
		{att.texcoords[idx0*2 + 0],
		 att.texcoords[idx0*2 + 1]},
		{att.texcoords[idx1*2 + 0],
		 att.texcoords[idx1*2 + 1]},
		{att.texcoords[idx2*2 + 0],
		 att.texcoords[idx2*2 + 1]}
	};
};


Vec3f Core::shootRay(const Vec3f& pos, const Vec3f& dir) {
	RTCRayHit rayhit;

	rayhit.ray.org_x = pos[0];
	rayhit.ray.org_y = pos[1];
	rayhit.ray.org_z = pos[2];
	rayhit.ray.dir_x = dir[0];
	rayhit.ray.dir_y = dir[1];
	rayhit.ray.dir_z = dir[2];
	rayhit.ray.flags = 0;
	rayhit.ray.tnear = 0;
	rayhit.ray.tfar = 1;
	rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

	rtcIntersect1(hi_embree_scene, &hi_embree_context, &rayhit);

	if(rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) {
		rayhit.ray.dir_x = -dir[0];
		rayhit.ray.dir_y = -dir[1];
		rayhit.ray.dir_z = -dir[2];
		rayhit.ray.flags = 0;
		rayhit.ray.tnear = 0;
		rayhit.ray.tfar = 1;
		rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

		rtcIntersect1(hi_embree_scene, &hi_embree_context, &rayhit);

		// This should never happen
		if(rayhit.hit.geomID == RTC_INVALID_GEOMETRY_ID) return {0,0,0};
	}

	const uint id = rayhit.hit.primID;
	const float a1 = rayhit.hit.u;
	const float a2 = rayhit.hit.v;
	const float a0 = 1 - a1 - a2;

	const int n0_idx = hi_shape.mesh.indices[id*3 + 0].normal_index;
	const int n1_idx = hi_shape.mesh.indices[id*3 + 1].normal_index;
	const int n2_idx = hi_shape.mesh.indices[id*3 + 2].normal_index;
	const Vec3f n0{	hi_attrib.normals[n0_idx*3 + 0],
					hi_attrib.normals[n0_idx*3 + 1],
					hi_attrib.normals[n0_idx*3 + 2]};
	const Vec3f n1{	hi_attrib.normals[n1_idx*3 + 0],
					hi_attrib.normals[n1_idx*3 + 1],
					hi_attrib.normals[n1_idx*3 + 2]};
	const Vec3f n2{	hi_attrib.normals[n2_idx*3 + 0],
					hi_attrib.normals[n2_idx*3 + 1],
					hi_attrib.normals[n2_idx*3 + 2]};

	return {a0*n0 + a1*n1 + a2*n2};
}