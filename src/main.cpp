#include <iostream>

#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>

#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

#include <QtWidgets>

#include "mainWindow.hpp"

#define VERBOSE 1

void loadObj(	std::string						inputfile, 
				tinyobj::attrib_t&				attrib,
				std::vector<tinyobj::shape_t>&	shapes);

void setupEmbree(	RTCDevice&				dev, 
					RTCScene&				scene, 
					RTCIntersectContext&	ray_context,
					std::vector<float>&		vertices,
					std::vector<uint>&		triangles);

int main(int argc, char** argv) {
	QApplication app(argc, argv);

	// load gui
	QImage buffer{300, 300, QImage::Format_RGB888};
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
	RTCIntersectContext embree_ray_context;
	setupEmbree(embree_device, embree_scene, embree_ray_context, attrib.vertices, triangles);

	// cast


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
					RTCIntersectContext&	ray_context,
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

	// This data structure is needed by embree. It is never read in this code.
	rtcInitIntersectContext(&ray_context);
}