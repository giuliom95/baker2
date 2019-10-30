#ifndef _CORE_HPP_
#define _CORE_HPP_

#ifndef VERBOSE
	#define VERBOSE 1
#endif

#include "tiny_obj_loader.h"

#include <iostream>


#include <xmmintrin.h>
#include <pmmintrin.h>
#include <embree3/rtcore.h>

// For QImage
#include <QWidget>

#include "math.hpp"

#define DEF_TEX_SIZE 2048

// The square root of the number of samples
#define DEF_SPP_SIDE 2

class Triangle {
public:
	static Triangle fromIndex(	const int ti,
								const tinyobj::shape_t& shape,
								const tinyobj::attrib_t& att);

	const Vec3f p0,		p1,		p2;
	const Vec3f n0,		n1,		n2;
	const Vec2f uv0,	uv1,	uv2;
};

// TODO: Make indipendent from QImage
class Core {
public:
	Core();
	~Core();

	void loadLowObj	(std::string filename);
	void loadHighObj(std::string filename);

	void clearBuffers();
	void generateNormalMapOnTriangle(const int ti);
	void generateNormalMap();
	void divideMapByCount();

	int tex_w, tex_h;
	std::vector<float> tex;

	const int getLowTrisNum();

private:

	tinyobj::attrib_t	low_attrib;
	tinyobj::shape_t	low_shape;

	tinyobj::attrib_t	hi_attrib;
	tinyobj::shape_t	hi_shape;

	RTCScene			hi_embree_scene;
	RTCDevice			hi_embree_device;
	RTCIntersectContext hi_embree_context;

	std::vector<int> pix_count;

	void setupEmbree();
	void releaseEmbree();

	bool shootRay(const Vec3f& pos, const Vec3f& dir, Vec3f& n);

	Vec3f toTangSpace(	const Vec3f&	hi_n,
						const Vec3f&	low_p, 
						const Vec3f&	low_n,
						const Vec2f&	uv,
						const Triangle&	t);

	static void loadObj(std::string						inputfile, 
						tinyobj::attrib_t&				attrib,
						std::vector<tinyobj::shape_t>&	shapes);

	// TODO: Remove later
	static void bbox(	const std::vector<float>&	vertices,
						Vec3f&						min,
						Vec3f&						max);
};

#endif
