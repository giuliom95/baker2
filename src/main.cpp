#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <QtWidgets>

#include "mainWindow.hpp"

#define VERBOSE 1

using Vec3f = std::array<float, 3>;
Vec3f normalized(float x, float y, float z) {
	const float l = std::sqrt(x*x + y*y + z*z);
	return {x / l, y /l, z / l};
}

using Vec2f = std::array<float, 2>;


std::ostream& operator<<(std::ostream& s, const Vec3f& v) {
	s << "(" << v[0] << ", " << v[1] << ", " << v[2] << ")";
	return s;
}

void loadObj(		std::string						inputfile, 
					tinyobj::attrib_t&				attrib,
					std::vector<tinyobj::shape_t>&	shapes);

void bbox(			const std::vector<float>&		vertices,
					Vec3f&							min,
					Vec3f&							max);

void setupEmbree(	RTCDevice&						dev, 
					RTCScene&						scene,
					std::vector<float>&				vertices,
					std::vector<uint>&				triangles);

void raycast(		QImage&							out, 
					const RTCScene&					scene, 
					const Vec3f&					bbox_min, 
					const Vec3f&					bbox_max);

int main(int argc, char** argv) {
	QApplication app(argc, argv);

	// load gui
	QImage buffer{1024, 1024, QImage::Format_RGB888};
	auto* bufferData = buffer.bits();
	for(int i = 0; i < buffer.width(); ++i)
		for(int j = 0; j < buffer.height(); ++j) {
			bufferData[3*(j*buffer.width() + i) + 0] = (uchar)(256*((float)i / buffer.width()));
			bufferData[3*(j*buffer.width() + i) + 1] = (uchar)(256*((float)j / buffer.height()));
			bufferData[3*(j*buffer.width() + i) + 2] = 0;
		}

	MainWindow win{buffer};
	win.show();

	// load obj
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	loadObj("in/head.obj", attrib, shapes);

	// Data convert from tinyobjloader to embree
	int noIndices = shapes[0].mesh.indices.size();
	std::vector<uint> triangles(noIndices);
	for(int i = 0; i < noIndices; ++i)
		triangles[i] = (uint)(shapes[0].mesh.indices[i].vertex_index);

	// upload data to embree
	RTCDevice embree_device;
	RTCScene embree_scene;
	setupEmbree(embree_device, embree_scene, attrib.vertices, triangles);

	// Compute bbox
	Vec3f bbox_min, bbox_max;
	bbox(attrib.vertices, bbox_min, bbox_max);

	// cast
	raycast(buffer, embree_scene, bbox_min, bbox_max);

	win.setImage(buffer);

	// Free Embree resources
	rtcReleaseScene(embree_scene);
	rtcReleaseDevice(embree_device);

	return app.exec();
}


void loadObj(	std::string						inputfile, 
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

void setupEmbree(	RTCDevice&				dev, 
					RTCScene&				scene,
					std::vector<float>&		vertices,
					std::vector<uint>&		triangles) {
	// Activation of "Flush to Zero" and "Denormals are Zero" CPU modes.
	// Embree reccomends them in sake of performance.
	// The #ifndef is needed to make VSCode Intellisense ignore these lines. 
	// It believes that _MM_SET_DENORMALS_ZERO_MODE and _MM_DENORMALS_ZERO_ON are defined nowhere.
	#ifndef __INTELLISENSE__
		_MM_SET_FLUSH_ZERO_MODE(_MM_FLUSH_ZERO_ON);
		_MM_SET_DENORMALS_ZERO_MODE(_MM_DENORMALS_ZERO_ON);
	#endif // __INTELLISENSE__

	dev = rtcNewDevice(VERBOSE ? "verbose=3" : "verbose=1");
	scene = rtcNewScene(dev);

	const auto geom = rtcNewGeometry(dev, RTC_GEOMETRY_TYPE_TRIANGLE);
	rtcSetSharedGeometryBuffer(	geom, RTC_BUFFER_TYPE_VERTEX, 0, 
								RTC_FORMAT_FLOAT3, vertices.data(),
								0, 3*sizeof(float), vertices.size() / 3);
	rtcSetSharedGeometryBuffer(	geom, RTC_BUFFER_TYPE_INDEX, 0, 
								RTC_FORMAT_UINT3, triangles.data(),
								0, 3*sizeof(uint), triangles.size() / 3);
	rtcCommitGeometry(geom);
	const auto geom_id = rtcAttachGeometry(scene, geom);
	rtcReleaseGeometry(geom);
	rtcCommitScene(scene);

}

void bbox(const std::vector<float>& vertices, Vec3f& min, Vec3f& max) {
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

void raycast(	QImage&			out, 
				const RTCScene&	scene, 
				const Vec3f&	bbox_min, 
				const Vec3f&	bbox_max) {

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
	
	const uint& out_w = out.width();
	const uint& out_h = out.height();
	uchar* out_data = out.bits();

	for(uint i = 0; i < out_w; ++i) {
		for(uint j = 0; j < out_h; ++j) {

			// Map pixel into bbox XY surface
			rayhit.ray.org_x = scene_dx * ((float)i / out_w) + bbox_min[0];
			rayhit.ray.org_y = scene_dy * ((float)j / out_h) + bbox_min[1];

			//std::cout << rayhit.ray.org_x << " " << rayhit.ray.org_y << std::endl;

			rayhit.ray.flags = 0;
			rayhit.ray.tnear = 0;
			rayhit.ray.tfar = scene_dz;

			rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;

			rtcIntersect1(scene, &context, &rayhit);

			out_data[3*(i + j*out_w) + 0] = 0;
			out_data[3*(i + j*out_w) + 1] = 0;
			out_data[3*(i + j*out_w) + 2] = 0;

			if(rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
				
				Vec3f n = normalized(rayhit.hit.Ng_x, rayhit.hit.Ng_y, rayhit.hit.Ng_z);
				out_data[3*(i + j*out_w) + 0] = 255 * 0.5 * (n[0] + 1);
				out_data[3*(i + j*out_w) + 1] = 255 * 0.5 * (n[1] + 1);
				out_data[3*(i + j*out_w) + 2] = 255 * 0.5 * (n[2] + 1);
			};
		}
	}
}

