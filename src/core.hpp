#ifndef _CORE_HPP_
#define _CORE_HPP_

#ifndef VERBOSE
	#define VERBOSE 1
#endif

#include <tiny_obj_loader.h>

#include <iostream>


#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

// For QImage
#include <QWidget>

#include "math.hpp"

// TODO: Make indipendent from QImage
class Core {
public:
	Core();
	~Core();

	void loadLowObj	(std::string filename);
	void loadHighObj(std::string filename);

	void raycast();

	QImage texture;

private:
	tinyobj::attrib_t	low_attrib;
	tinyobj::shape_t	low_shape;

	tinyobj::attrib_t	hi_attrib;
	tinyobj::shape_t	hi_shape;

	RTCScene			hi_embree_scene;
	RTCDevice			hi_embree_device;

	void setupEmbree();
	void releaseEmbree();

	static void loadObj(		std::string						inputfile, 
					tinyobj::attrib_t&				attrib,
					std::vector<tinyobj::shape_t>&	shapes);

	// TODO: Remove later
	static void bbox(	const std::vector<float>&	vertices,
						Vec3f&						min,
						Vec3f&						max);

};

#endif