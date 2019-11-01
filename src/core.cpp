#include "core.hpp"

Core::Core() :
	tex_w{DEF_TEX_SIZE}, tex_h{DEF_TEX_SIZE},
	pix_count(DEF_TEX_SIZE*DEF_TEX_SIZE),
	tex(3*DEF_TEX_SIZE*DEF_TEX_SIZE),
	hi_embree_scene{nullptr},
	hi_embree_device{nullptr} {
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

	std::string err, warn;

	bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, inputfile.c_str());

	if(!ret)
		std::cerr << err << std::endl;

	if(attrib.normals.size() == 0) {
		// need to generate vertex normals
		std::cout << "Generating normals..." << std::endl;

		// iterate over triangles to get their face normals

		// iterate over vertices of every triangle and average the normals of the adjacent faces

		// to find adjacent faces to a vertex, iterate over faces and look for the vertex index occurencies
		tinyobj::shape_t& s = shapes[0];

		const int trinum{s.mesh.indices.size() / 3};
		const int vnum{attrib.vertices.size() / 3};
		// Initialize blank normals
		attrib.normals = std::vector<tinyobj::real_t>(vnum*3, 0);

		for (int ti = 0; ti < trinum; ++ti) {
			const int vidx0 = s.mesh.indices[ti*3 + 0].vertex_index;
			const int vidx1 = s.mesh.indices[ti*3 + 1].vertex_index;
			const int vidx2 = s.mesh.indices[ti*3 + 2].vertex_index;
			s.mesh.indices[ti*3 + 0].normal_index = vidx0;
			s.mesh.indices[ti*3 + 1].normal_index = vidx1;
			s.mesh.indices[ti*3 + 2].normal_index = vidx2;

			const Vec3f p0	{attrib.vertices[3*vidx0 + 0],
							 attrib.vertices[3*vidx0 + 1],
							 attrib.vertices[3*vidx0 + 2]};
			const Vec3f p1	{attrib.vertices[3*vidx1 + 0],
							 attrib.vertices[3*vidx1 + 1],
							 attrib.vertices[3*vidx1 + 2]};
			const Vec3f p2	{attrib.vertices[3*vidx2 + 0],
							 attrib.vertices[3*vidx2 + 1],
							 attrib.vertices[3*vidx2 + 2]};
			const Vec3f p01{p1 - p0};
			const Vec3f p02{p2 - p0};
			const Vec3f n{normalize(cross(p01, p02))};

			attrib.normals[3*vidx0 + 0] += n[0];
			attrib.normals[3*vidx0 + 1] += n[1];
			attrib.normals[3*vidx0 + 2] += n[2];
			attrib.normals[3*vidx1 + 0] += n[0];
			attrib.normals[3*vidx1 + 1] += n[1];
			attrib.normals[3*vidx1 + 2] += n[2];
			attrib.normals[3*vidx2 + 0] += n[0];
			attrib.normals[3*vidx2 + 1] += n[1];
			attrib.normals[3*vidx2 + 2] += n[2];
		}

		for(int vi = 0; vi < vnum; ++vi) {
			const Vec3f n{normalize({attrib.normals[3*vi + 0],
									 attrib.normals[3*vi + 1],
									 attrib.normals[3*vi + 2]})};
			attrib.normals[3*vi + 0] = n[0];
			attrib.normals[3*vi + 1] = n[1];
			attrib.normals[3*vi + 2] = n[2];

			//std::cout << n << std::endl;
		}

		std::cout << "Normals generated." << std::endl;
	}

	// Verbose info printing
	if(VERBOSE) {
		// Print vertices
		std::cout << "vnum: " << (attrib.vertices.size() / 3) << std::endl;
		std::cout << "nnum: " << (attrib.normals.size() / 3) << std::endl;
		std::cout << "uvnum: " << (attrib.texcoords.size() / 2) << std::endl;
		std::cout << "indexes: " << shapes[0].mesh.indices.size() << std::endl;
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
	rtcAttachGeometry(hi_embree_scene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(hi_embree_scene);

	rtcInitIntersectContext(&hi_embree_context);
}

void Core::releaseEmbree() {
	std::cout << "Releasing Embree" << std::endl;
	if(hi_embree_scene) rtcReleaseScene(hi_embree_scene);
	if(hi_embree_device) rtcReleaseDevice(hi_embree_device);
}

void Core::clearBuffers() {
	pix_count = std::vector<int>(tex_w*tex_h, 0),
	tex = std::vector<float>(3*tex_w*tex_w, 0);
}

void Core::generateNormalMap() {

	// For each triangle
	const auto trinum = getLowTrisNum();
	for (int ti = 0; ti < trinum; ++ti) {
		generateNormalMapOnTriangle(ti);
	}

	divideMapByCount();
	
}

void Core::generateNormalMapOnTriangle(const int ti) {

	const Triangle t = Triangle::fromIndex(ti, low_shape, low_attrib);
		
	const Vec2i uv0i{tex_w * t.uv0[0], tex_h * t.uv0[1]};
	const Vec2i uv1i{tex_w * t.uv1[0], tex_h * t.uv1[1]};
	const Vec2i uv2i{tex_w * t.uv2[0], tex_h * t.uv2[1]};

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
			//std::cout << "texel " << i << " " << j << std::endl;
			for(int us = 0; us < DEF_SPP_SIDE; ++us) {
				for(int vs = 0; vs < DEF_SPP_SIDE; ++vs) {

					// Check if current point is inside
					const Vec2f uv = {	(i + ((float)us / DEF_SPP_SIDE)) / tex_w,
										(j + ((float)vs / DEF_SPP_SIDE)) / tex_h};
					const Vec2f uvt = mat * (uv - t.uv0);

					const float ct = 1 - uvt[0] - uvt[1];
					const bool inside = uvt[0]	>= 0 && uvt[0]	< 1 &&
										uvt[1]	>= 0 && uvt[1]	< 1 &&
										ct		>= 0 && ct		< 1;

					if(inside) {
						
						const Vec3f pos = ct*t.p0 + uvt[0]*t.p1 + uvt[1]*t.p2;
						const Vec3f dir = ct*t.n0 + uvt[0]*t.n1 + uvt[1]*t.n2;

						Vec3f n;
						Vec3f tn;	// Normal in tangent space.
						bool hit{shootRay(pos, dir, n)};

						bool wrong_way{true};
						if(hit) {
							tn = toTangSpace(n, pos, dir, uv, t);
							wrong_way = dot(tn, {0,0,1}) < 0;
						}

						if(wrong_way || !hit) {
							hit = shootRay(pos, -1*dir, n);
							tn = toTangSpace(n, pos, dir, uv, t);

							wrong_way = dot(tn, {0,0,1}) < 0;
						}

						if(hit && !wrong_way) {

							// Color texture
							tex[3*(i + j*tex_w) + 0] += tn[0];
							tex[3*(i + j*tex_w) + 1] += tn[1];
							tex[3*(i + j*tex_w) + 2] += tn[2];

							pix_count[i + j*tex_w] += 1;
						}

					}
				}
			}
		}
	}
}

void Core::divideMapByCount() {
	for(int j = 0; j < tex_h; ++j) {
		for(int i = 0; i < tex_w; ++i) {
			const int count = pix_count[i + j*tex_w];
			if(count > 0) {
				tex[3*(i + j*tex_w) + 0] /= count;
				tex[3*(i + j*tex_w) + 1] /= count;
				tex[3*(i + j*tex_w) + 2] /= count;
			} else {
				tex[3*(i + j*tex_w) + 0] = 0;
				tex[3*(i + j*tex_w) + 1] = 0;
				tex[3*(i + j*tex_w) + 2] = 1;
			}
		}
	}
}

const int Core::getLowTrisNum() {
	return low_shape.mesh.indices.size() / 3;
}


Triangle Triangle::fromIndex(	const int ti,
								const tinyobj::shape_t& shape,
								const tinyobj::attrib_t& att) {

	const int vidx0 =	shape.mesh.indices[ti*3 + 0].vertex_index;
	const int vidx1 =	shape.mesh.indices[ti*3 + 1].vertex_index;
	const int vidx2 =	shape.mesh.indices[ti*3 + 2].vertex_index;
	const int nidx0 = 	shape.mesh.indices[ti*3 + 0].normal_index;
	const int nidx1 = 	shape.mesh.indices[ti*3 + 1].normal_index;
	const int nidx2 = 	shape.mesh.indices[ti*3 + 2].normal_index;
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


bool Core::shootRay(const Vec3f& pos, const Vec3f& dir, Vec3f& n) {
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

	const uint  id{rayhit.hit.primID};
	const float a1{rayhit.hit.u};
	const float a2{rayhit.hit.v};
	const float a0{1 - a1 - a2};

	const int n0_idx{hi_shape.mesh.indices[id*3 + 0].normal_index};
	const int n1_idx{hi_shape.mesh.indices[id*3 + 1].normal_index};
	const int n2_idx{hi_shape.mesh.indices[id*3 + 2].normal_index};
	const Vec3f n0{	hi_attrib.normals[n0_idx*3 + 0],
					hi_attrib.normals[n0_idx*3 + 1],
					hi_attrib.normals[n0_idx*3 + 2]};
	const Vec3f n1{	hi_attrib.normals[n1_idx*3 + 0],
					hi_attrib.normals[n1_idx*3 + 1],
					hi_attrib.normals[n1_idx*3 + 2]};
	const Vec3f n2{	hi_attrib.normals[n2_idx*3 + 0],
					hi_attrib.normals[n2_idx*3 + 1],
					hi_attrib.normals[n2_idx*3 + 2]};

	n = {a0*n0 + a1*n1 + a2*n2};

	return rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID; 
}

Vec3f Core::toTangSpace(const Vec3f&	hi_n,
						const Vec3f&	low_p, 
						const Vec3f&	low_n,
						const Vec2f&	uv,
						const Triangle&	t) {

	// Get point on +u
	const Vec2f new_uv	{uv[0] + 0.01, uv[1]};
	const float t_area	{triarea( t.uv0,  t.uv1,  t.uv2)};
	const float a		{triarea(new_uv,  t.uv1,  t.uv2) / t_area};
	const float b		{triarea( t.uv0, new_uv,  t.uv2) / t_area};
	const float c		{triarea( t.uv0,  t.uv1, new_uv) / t_area};
	const Vec3f new_p	{a*t.p0 + b*t.p1 + c*t.p2};
	const Vec3f tmp_tang{new_p - low_p};
	// Build tangent space to world space reference frame
	const Vec3f bitang	{normalize(cross(low_n, tmp_tang))};
	const Vec3f tang	{cross(bitang, low_n)};
	const Mat4  mat		{tang, bitang, low_n, {}};

	// Hi poly normal into tangent space of low poly model
	return {transformVector(transpose(mat), hi_n)};

}
