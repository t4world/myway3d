#include "MWRenderer.h"
#include "MWMaterialManager.h"

using namespace Myway;

Renderer::Renderer()
{
	mUsingDefferedShading = true;
}

Renderer::~Renderer()
{
}

const Aabb & Renderer::GetWorldAabb()
{
	return Aabb::Identiy;
}

void Renderer::GetWorldPosition(Vec3 * pos)
{
	*pos = Vec3::Zero;
}

void Renderer::GetWorldTransform(Mat4 * form)
{
	*form = Mat4::Identity;
}

int Renderer::GetBlendMatrix(Mat4 * forms)
{
	return 0;
}

void Renderer::BeginRendering()
{
}

void Renderer::EndRendering()
{
}

void Renderer::SetPrimitiveCount(int size)
{
    mPrimCount = size;
}

void Renderer::SetPrimitiveType(PRIMITIVE_TYPE type)
{
    mPrimType = type;
}

VertexStream * Renderer::GetVertexStream()
{
    return &mVertexStream;
}

IndexStream * Renderer::GetIndexStream()
{
    return &mIndexStream;
}

int Renderer::GetPrimitiveCount() const
{
    return mPrimCount;
}

PRIMITIVE_TYPE Renderer::GetPrimitiveType() const
{
    return mPrimType;
}

void Renderer::SetMaterial(const Material * material)
{
    mMaterial = *material;
}

Material * Renderer::GetMaterial()
{
    return &mMaterial;
}
