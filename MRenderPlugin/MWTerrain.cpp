#include "MWTerrain.h"
#include "MWTerrainLod.h"
#include "MWTerrainSection.h"
#include "MWWorld.h"
#include "MWImage.h"
#include "MWResourceManager.h"
#include "MWEnvironment.h"
#include "MWRenderEvent.h"
#include "MWRenderHelper.h"

namespace Myway {

const int K_Terrain_Version = 0;
const int K_Terrain_Magic = 'TRN0';

#define _POSITION    0
#define _HEIGHT		 1
#define _NORMAL      2
#define _MORPH       3

Terrain::Terrain(const Config & config)
	: tOnPreVisibleCull(RenderEvent::OnPreVisibleCull, this, &Terrain::OnPreVisibleCull)
{
	mInited = false;
	mLockedData = NULL;
	mLockedWeightMapData = NULL;

	mLod = new TerrainLod(kMaxDetailLevel);

	for (int i = 0; i < kMaxDetailLevel; ++i)
	{
		mTech[i] = Environment::Instance()->GetShaderLib()->GetTechnique(TString128("TerrainL") + (i + 1));
		mTechMirror[i] = Environment::Instance()->GetShaderLib()->GetTechnique(TString128("TerrainM") + (i + 1));
		
		d_assert (mTech[i] && mTechMirror[i]);
	}

	mVertexDecl = VideoBufferManager::Instance()->CreateVertexDeclaration();
	mVertexDecl->AddElement(_POSITION, 0,  DT_FLOAT2, DU_POSITION, 0);
	mVertexDecl->AddElement(_HEIGHT, 0,  DT_FLOAT1, DU_TEXCOORD, 0);
	mVertexDecl->AddElement(_NORMAL, 0,  DT_COLOR, DU_NORMAL, 0);
	mVertexDecl->AddElement(_MORPH, 0, DT_FLOAT1, DU_BLENDWEIGHT, 0);
	mVertexDecl->Init();

	_Create(config);
}

Terrain::Terrain(const char * source)
	: tOnPreVisibleCull(RenderEvent::OnPreVisibleCull, this, &Terrain::OnPreVisibleCull)
{
	mInited = false;
	mLockedData = NULL;
	mLockedWeightMapData = NULL;

	mLod = new TerrainLod(kMaxDetailLevel);

	for (int i = 0; i < kMaxDetailLevel; ++i)
	{
		mTech[i] = Environment::Instance()->GetShaderLib()->GetTechnique(TString128("TerrainL") + (i + 1));
		mTechMirror[i] = Environment::Instance()->GetShaderLib()->GetTechnique(TString128("TerrainM") + (i + 1));

		d_assert (mTech[i] && mTechMirror[i]);
	}

	mVertexDecl = VideoBufferManager::Instance()->CreateVertexDeclaration();
	mVertexDecl->AddElement(_POSITION, 0,  DT_FLOAT2, DU_POSITION, 0);
	mVertexDecl->AddElement(_HEIGHT, 0,  DT_FLOAT1, DU_TEXCOORD, 0);
	mVertexDecl->AddElement(_NORMAL, 0,  DT_COLOR, DU_NORMAL, 0);
	mVertexDecl->AddElement(_MORPH, 0, DT_FLOAT1, DU_BLENDWEIGHT, 0);
	mVertexDecl->Init();

	_Load(source);
}


Terrain::~Terrain()
{
	d_assert(mLockedData == NULL);
	d_assert(mLockedWeightMapData == NULL);

	safe_delete (mLod);

	for (int i = 0; i < mSections.Size(); ++i)
	{
		delete mSections[i];
		World::Instance()->DestroySceneNode(mSceneNodes[i]);
	}

	mSceneNodes.Clear();
	mSections.Clear();

	safe_delete_array(mHeights);
	safe_delete_array(mNormals);
	safe_delete_array(mWeights);
}

void Terrain::_init()
{
	// create shared x & y stream
	mXYStream = VideoBufferManager::Instance()->CreateVertexBuffer(8 * kSectionVertexSize * kSectionVertexSize, 8);

	float * vert = (float *)mXYStream->Lock(0, 0, LOCK_NORMAL);
	{
		float w = mConfig.xSize / mConfig.xSectionCount;
		float h = mConfig.zSize / mConfig.zSectionCount;

		for (int j = 0; j < kSectionVertexSize; ++j)
		{
			for (int i = 0; i < kSectionVertexSize; ++i)
			{
				*vert++ = i / (float)(kSectionVertexSize - 1) * w;
				*vert++ = (1 - j / (float)(kSectionVertexSize - 1)) * h;
			}
		}
	}
	mXYStream->Unlock();

	// create sections
	mSections.Resize(mConfig.xSectionCount * mConfig.zSectionCount);
	mSceneNodes.Resize(mConfig.xSectionCount * mConfig.zSectionCount);

	int index = 0;
	for (int j = 0; j < mConfig.zSectionCount; ++j)
	{
		for (int i = 0; i < mConfig.xSectionCount; ++i)
		{
			mSections[index] = new TerrainSection(this, i, j);
			mSceneNodes[index] = World::Instance()->CreateSceneNode();

			mSceneNodes[index]->Attach(mSections[index]);

			++index;
		}
	}

	// create weight map.
	mWeightMaps.Resize(mConfig.iSectionCount);

	index = 0;
	
	for (int j = 0; j < mConfig.zSectionCount; ++j)
	{
		for (int i = 0; i < mConfig.xSectionCount; ++i)
		{

			TString128 texName = TString128("TWeightMap_") + j + "_" + i; 
			TexturePtr texture = VideoBufferManager::Instance()->CreateTexture(texName, kWeightMapSize, kWeightMapSize, 0, FMT_A8R8G8B8);

			mWeightMaps[index++] = texture;
		}
	}

	for (int i = 0; i < mSections.Size(); ++i)
	{
		TerrainSection * section = mSections[i];

		int x = section->GetSectionX();
		int z = section->GetSectionZ();

		TexturePtr weightMap = GetWeightMap(x, z);

		LockedBox lb;
		weightMap->Lock(0, &lb, NULL, LOCK_DISCARD);

		Color * data = mWeights + z * kWeightMapSize * mConfig.xWeightMapSize + x * kWeightMapSize;
		int * dest = (int *)lb.pData;
		for (int k = 0; k < kWeightMapSize; ++k)
		{
			for (int p = 0; p < kWeightMapSize; ++p)
				dest[p] = M_RGBA(data[p].r, data[p].g, data[p].b, data[p].a);

			dest += kWeightMapSize;
			data += mConfig.xWeightMapSize;
		}

		weightMap->Unlock(0);
	}

	// load default detail map
	mDefaultDetailMap = VideoBufferManager::Instance()->Load2DTexture("Terrain\\TerrainDefault.png", "Terrain\\TerrainDefault.png");
	mDefaultNormalMap = RenderHelper::Instance()->GetDefaultNormalTexture();

	for (int i = 0; i < kMaxLayers; ++i)
	{
		mDetailMaps[i] = mDefaultDetailMap;
		mNormalMaps[i] = mDefaultNormalMap;
		mSpecularMaps[i] = RenderHelper::Instance()->GetWhiteTexture();
	}

	for (int i = 0; i < kMaxLayers; ++i)
	{
		if (mLayer[i].detail != "")
			mDetailMaps[i] = VideoBufferManager::Instance()->Load2DTexture(mLayer[i].detail, mLayer[i].detail);

		if (mLayer[i].normal != "")
			mNormalMaps[i] = VideoBufferManager::Instance()->Load2DTexture(mLayer[i].normal, mLayer[i].normal);

		if (mLayer[i].specular != "")
			mSpecularMaps[i] = VideoBufferManager::Instance()->Load2DTexture(mLayer[i].specular, mLayer[i].specular);
	}
}

void Terrain::_calcuNormals()
{
	d_assert (mNormals == NULL);

	mNormals = new Color[mConfig.iVertexCount];

	for (int j = 0; j < mConfig.zVertexCount; ++j)
	{
		for (int i = 0; i < mConfig.xVertexCount; ++i)
		{
			Vec3 a = _getPosition(i - 1, j + 0);
			Vec3 b = _getPosition(i + 0, j - 1);
			Vec3 c = _getPosition(i + 1, j + 0);
			Vec3 d = _getPosition(i + 0, j + 1);
			Vec3 p = _getPosition(i + 0, j + 0);

			Vec3 L = a - p, T = b - p, R = c - p, B = d - p;

			Vec3 N = Vec3::Zero;
			float len_L = L.Length(), len_T = T.Length();
			float len_R = R.Length(), len_B = B.Length();

			if (len_L > 0.01f && len_T > 0.01f)
			N += L.CrossN(T);

			if (len_T > 0.01f && len_R > 0.01f)
			N += T.CrossN(R);

			if (len_R > 0.01f && len_B > 0.01f)
			N += R.CrossN(B);

			if (len_B > 0.01f && len_L > 0.01f)
			N += B.CrossN(L);

			N.NormalizeL();
			N = (N + Vec3::Unit) / 2 * 255.0f;
			
			unsigned char cr = (unsigned char)N.x;
			unsigned char cg = (unsigned char)N.y;
			unsigned char cb = (unsigned char)N.z;
			unsigned char ca = 255;

			mNormals[j * mConfig.xVertexCount + i] = Color(cr, cg, cb, ca);
		}
	}
}

void Terrain::_Create(const Config & config)
{
	d_assert (!mInited);

	mConfig = config;

	int x = config.xVertexCount - 1;
	int z = config.zVertexCount - 1;
	int k = kSectionVertexSize - 1;

	if ((x % k) != 0)
		mConfig.xVertexCount = (x / k + 1) * k + 1;

	if ((z % k) != 0)
		mConfig.zVertexCount = (z / k + 1) * k + 1;

	mConfig.iVertexCount = mConfig.xVertexCount * mConfig.zVertexCount;

	mConfig.xSectionCount = mConfig.xVertexCount / (kSectionVertexSize - 1);
	mConfig.zSectionCount = mConfig.zVertexCount / (kSectionVertexSize - 1);

	mConfig.iSectionCount = mConfig.xSectionCount * mConfig.zSectionCount;

	mConfig.xSectionSize = mConfig.xSize / mConfig.xSectionCount;
	mConfig.zSectionSize = mConfig.zSize / mConfig.zSectionCount;

	// init default heights and normals
	mHeights = new float[mConfig.iVertexCount];
	for (int i = 0; i < mConfig.iVertexCount; ++i)
		mHeights[i] = 0;

	mNormals = new Color[mConfig.iVertexCount];
	for (int i = 0; i < mConfig.iVertexCount; ++i)
		mNormals[i] = Color(127, 255, 127);

	// create weight maps
	mConfig.xWeightMapSize = Terrain::kWeightMapSize * mConfig.xSectionCount;
	mConfig.zWeightMapSize = Terrain::kWeightMapSize * mConfig.zSectionCount;
	mConfig.iWeightMapSize = mConfig.xWeightMapSize * mConfig.zWeightMapSize;

	mWeights = new Color[mConfig.xWeightMapSize * mConfig.zWeightMapSize];
	for (int i = 0; i < mConfig.xWeightMapSize * mConfig.zWeightMapSize; ++i)
	{
		mWeights[i] = Color(0, 0, 0, 255);
	}

	mBound.minimum = Vec3(0, 0, 0);
	mBound.maximum = Vec3(mConfig.xSize, 0, mConfig.zSize);

	_init();

	mInited = true;
}

void Terrain_ReadConfig(Terrain::Config * cfg, DataStreamPtr file, int version)
{
	file->Read(&cfg->xSize, sizeof(int));
	file->Read(&cfg->zSize, sizeof(int));

	file->Read(&cfg->xVertexCount, sizeof(int));
	file->Read(&cfg->zVertexCount, sizeof(int));

	file->Read(&cfg->iVertexCount, sizeof(int));

	file->Read(&cfg->morphEnable, sizeof(bool));
	file->Read(&cfg->morphStart, sizeof(float));

	file->Read(&cfg->xSectionSize, sizeof(int));
	file->Read(&cfg->zSectionSize, sizeof(int));
	file->Read(&cfg->xSectionCount, sizeof(int));
	file->Read(&cfg->zSectionCount, sizeof(int));
	file->Read(&cfg->iSectionCount, sizeof(int));

	file->Read(&cfg->xWeightMapSize, sizeof(int));
	file->Read(&cfg->zWeightMapSize, sizeof(int));
	file->Read(&cfg->iWeightMapSize, sizeof(int));

	file->Read(&cfg->phyFlags, sizeof(BFlag32));
}

void Terrain::_Load(const char * filename)
{
	d_assert (!mInited);

	DataStreamPtr stream = ResourceManager::Instance()->OpenResource(filename);

	d_assert (stream != NULL);

	int magic, version;
	int * sectionLayers = NULL;

	stream->Read(&magic, sizeof(int));

	d_assert (magic == K_Terrain_Magic);

	stream->Read(&version, sizeof(int));

	if (version == 0)
	{
		int count = 0;

		Terrain_ReadConfig(&mConfig, stream, version);
		count = stream->Read(mLayer, sizeof(Layer), kMaxLayers);

		count = stream->Read(&mBound, sizeof(Aabb));

		mHeights = new float[mConfig.iVertexCount];
		count = stream->Read(&mHeights[0], sizeof(float), mConfig.iVertexCount);

		mNormals = new Color[mConfig.iVertexCount];
		count = stream->Read(&mNormals[0], sizeof(Color), mConfig.iVertexCount);

		mWeights = new Color[mConfig.iWeightMapSize];
		count = stream->Read(&mWeights[0], sizeof(Color), mConfig.iWeightMapSize);

		sectionLayers = new int[mConfig.iSectionCount * kMaxBlendLayers];

		count = stream->Read(sectionLayers, kMaxBlendLayers * sizeof(int), mConfig.iSectionCount);
	}
	else
	{
		d_assert (0);
	}

	_init();

	if (sectionLayers)
	{
		int index = 0;
		for (int j = 0; j < mConfig.zSectionCount; ++j)
		{
			for (int i = 0; i < mConfig.xSectionCount; ++i)
			{
				TerrainSection * section = GetSection(i, j);

				for (int l = 0; l < kMaxBlendLayers; ++l)
					section->SetLayer(l, sectionLayers[index * kMaxBlendLayers + l]);

				++index;
			}
		}
	}

	safe_delete_array (sectionLayers);

	mInited = true;
}

void Terrain_WriteConfig(File & file, Terrain::Config * cfg)
{
	file.Write(&cfg->xSize, sizeof(int));
	file.Write(&cfg->zSize, sizeof(int));

	file.Write(&cfg->xVertexCount, sizeof(int));
	file.Write(&cfg->zVertexCount, sizeof(int));

	file.Write(&cfg->iVertexCount, sizeof(int));

	file.Write(&cfg->morphEnable, sizeof(bool));
	file.Write(&cfg->morphStart, sizeof(float));

	file.Write(&cfg->xSectionSize, sizeof(int));
	file.Write(&cfg->zSectionSize, sizeof(int));
	file.Write(&cfg->xSectionCount, sizeof(int));
	file.Write(&cfg->zSectionCount, sizeof(int));
	file.Write(&cfg->iSectionCount, sizeof(int));

	file.Write(&cfg->xWeightMapSize, sizeof(int));
	file.Write(&cfg->zWeightMapSize, sizeof(int));
	file.Write(&cfg->iWeightMapSize, sizeof(int));

	file.Write(&cfg->phyFlags, sizeof(BFlag32));
}

void Terrain::Save(const char * filename)
{
	File file;

	int count = 0;

	file.Open(filename, OM_WRITE_BINARY);

	file.Write(&K_Terrain_Magic, sizeof(int));
	file.Write(&K_Terrain_Version, sizeof(int));

	// write config
	Terrain_WriteConfig(file, &mConfig);

	// write layers
	count = file.Write(mLayer, sizeof(Layer), kMaxLayers);

	// write bound
	count = file.Write(&mBound, sizeof(Aabb));

	// write heights
	count = file.Write(mHeights, sizeof(float), mConfig.iVertexCount);

	// write normals
	count = file.Write(mNormals, sizeof(Color), mConfig.iVertexCount);
	
	// write weights
	count = file.Write(mWeights, sizeof(Color), mConfig.iWeightMapSize);

	// write section layer
	for (int j = 0; j < mConfig.zSectionCount; ++j)
	{
		for (int i = 0; i < mConfig.xSectionCount; ++i)
		{
			TerrainSection * section = GetSection(i, j);
			
			for (int l = 0; l < kMaxBlendLayers; ++l)
			{
				int layer = section->GetLayer(l);
				file.Write(&layer, sizeof(int));
			}
		}
	}

	file.Close();
}

int	Terrain::AddLayer(const Layer & layer)
{
	for (int i = 0; i < kMaxLayers; ++i)
	{
		if (mLayer[i].detail == "")
		{
			mLayer[i] = layer;

			mDetailMaps[i] = VideoBufferManager::Instance()->Load2DTexture(layer.detail, layer.detail);
			mNormalMaps[i] = VideoBufferManager::Instance()->Load2DTexture(layer.normal, layer.normal);
			mSpecularMaps[i] = VideoBufferManager::Instance()->Load2DTexture(layer.specular, layer.specular);

			return i;
		}
	}

	return -1;
}

const Terrain::Layer * Terrain::GetLayer(int index)
{
	d_assert (index < kMaxLayers);

	return &mLayer[index];
}

void Terrain::SetLayer(int index, const Layer & layer)
{
	d_assert (index < kMaxLayers);

	Layer & oldLayer = mLayer[index];

	if (oldLayer.detail != layer.detail)
		mDetailMaps[index] = VideoBufferManager::Instance()->Load2DTexture(layer.detail, layer.detail);

	if (layer.normal != "")
		mNormalMaps[index] = VideoBufferManager::Instance()->Load2DTexture(layer.normal, layer.normal);
	else
		mNormalMaps[index] = mDefaultNormalMap;

	if (layer.specular != "")
		mSpecularMaps[index] = VideoBufferManager::Instance()->Load2DTexture(layer.specular, layer.specular);
	else
		mSpecularMaps[index] = mDefaultSpecularMap;

	oldLayer = layer;
}

void Terrain::RemoveLayer(int layer)
{
	d_assert (layer < kMaxLayers);

	mLayer[layer].detail = "";
	mLayer[layer].normal = "";
	mLayer[layer].specular = "";
	mLayer[layer].material = -1;
	mLayer[layer].scale = 1;

	mDetailMaps[layer] = mDefaultDetailMap;
	mNormalMaps[layer] = mDefaultNormalMap;
	mSpecularMaps[layer] = RenderHelper::Instance()->GetWhiteTexture();
}

TerrainSection * Terrain::GetSection(int x, int z)
{
	d_assert (x < mConfig.xSectionCount && z < mConfig.zSectionCount);

	return mSections[z * mConfig.xSectionCount + x];
}

TexturePtr Terrain::GetWeightMap(int x, int z)
{
	d_assert (x < mConfig.xSectionCount && z < mConfig.zSectionCount);

	return mWeightMaps[z * mConfig.xSectionCount + x];
}

Vec3 Terrain::GetPosition(int x, int z)
{
	d_assert (x < mConfig.xVertexCount && z < mConfig.zVertexCount);

	float fx = (float)x / (mConfig.xVertexCount - 1) * mConfig.xSize;
	float fz = (1 - (float)z / (mConfig.zVertexCount - 1)) * mConfig.zSize;
	float fy = GetHeight(x, z);

	return Vec3(fx, fy, fz);
}

float Terrain::GetHeight(int x, int z)
{
	d_assert (x < mConfig.xVertexCount && z < mConfig.zVertexCount);

	return mHeights[z * mConfig.xVertexCount + x];
}

Color Terrain::GetNormal(int x, int z)
{
	d_assert (x < mConfig.xVertexCount && z < mConfig.zVertexCount);

	return mNormals[z * mConfig.xVertexCount + x];
}

Color Terrain::GetWeight(int x, int z)
{
	d_assert (x < mConfig.xWeightMapSize && z < mConfig.zWeightMapSize);

	return mWeights[z * mConfig.xWeightMapSize + x];
}

void Terrain::OnPreVisibleCull(Event * sender)
{
	mVisibleSections.Clear();
}

TexturePtr Terrain::_getDetailMap(int layer)
{
	if (layer >= 0 && layer < kMaxLayers)
		return mDetailMaps[layer];
	else
		return mDefaultDetailMap;
}

TexturePtr Terrain::_getNormalMap(int layer)
{
	if (layer >= 0 && layer < kMaxLayers)
		return mNormalMaps[layer];
	else
		return mDefaultNormalMap;
}

TexturePtr Terrain::_getSpecularMap(int layer)
{
	if (layer >= 0 && layer < kMaxLayers)
		return mSpecularMaps[layer];
	else
		return mDefaultSpecularMap;
}

Vec3 Terrain::_getPosition(int x, int z)
{
	x = Math::Maximum(0, x);
	z = Math::Maximum(0, z);

	x = Math::Minimum(x, mConfig.xVertexCount - 1);
	z = Math::Minimum(z, mConfig.zVertexCount - 1);

	return GetPosition(x, z);
}

float Terrain::_getHeight(int x, int z)
{
	x = Math::Maximum(0, x);
	z = Math::Maximum(0, z);

	x = Math::Minimum(x, mConfig.xVertexCount - 1);
	z = Math::Minimum(z, mConfig.zVertexCount - 1);

	return GetHeight(x, z);
}

Color Terrain::_getNormal(int x, int z)
{
	x = Math::Maximum(0, x);
	z = Math::Maximum(0, z);

	x = Math::Minimum(x, mConfig.xVertexCount - 1);
	z = Math::Minimum(z, mConfig.zVertexCount - 1);

	return GetNormal(x, z);
}

void Terrain::Render()
{
	RenderSystem * render = RenderSystem::Instance();

	for (int i = 0; i < mVisibleSections.Size(); ++i)
	{
		mVisibleSections[i]->UpdateLod();
	}

	ShaderParam * uTransform[kMaxDetailLevel];
	ShaderParam * uUVParam[kMaxDetailLevel];
	ShaderParam * uUVScale[kMaxDetailLevel];
	ShaderParam * uMorph[kMaxDetailLevel];

	for (int i = 0; i < kMaxDetailLevel; ++i)
	{
		uTransform[i] = mTech[i]->GetVertexShaderParamTable()->GetParam("gTransform");
		uUVParam[i] = mTech[i]->GetVertexShaderParamTable()->GetParam("gUVParam");
		uUVScale[i] = mTech[i]->GetVertexShaderParamTable()->GetParam("gUVScale");
		uMorph[i] = mTech[i]->GetVertexShaderParamTable()->GetParam("gMorph");
	}

	float xInvSectionSize = 1 / mConfig.xSectionSize;
	float zInvSectionSize = 1 / mConfig.zSectionSize;
	float xInvSize = 1 / mConfig.xSize;
	float zInvSize = 1 / mConfig.zSize;

	for (int i = 0; i < mVisibleSections.Size(); ++i)
	{
		TerrainSection * section = mVisibleSections[i];
		int x = section->GetSectionX();
		int z = section->GetSectionZ();
		int layer0 = section->GetLayer(0);
		int layer1 = section->GetLayer(1);
		int layer2 = section->GetLayer(2);
		int layer3 = section->GetLayer(3);
		float xOff = section->GetOffX();
		float zOff = section->GetOffZ();

		int techId = _getTechId(layer0, layer1, layer2, layer3);

		layer0 = Math::Maximum(0, layer0);
		layer1 = Math::Maximum(0, layer1);
		layer2 = Math::Maximum(0, layer2);
		layer3 = Math::Maximum(0, layer3);

		TexturePtr weightMap = GetWeightMap(x, z);
		TexturePtr detailMap0 = _getDetailMap(layer0);
		TexturePtr detailMap1 = _getDetailMap(layer1);
		TexturePtr detailMap2 = _getDetailMap(layer2);
		TexturePtr detailMap3 = _getDetailMap(layer3);
		TexturePtr normalMap0 = _getNormalMap(layer0);
		TexturePtr normalMap1 = _getNormalMap(layer1);
		TexturePtr normalMap2 = _getNormalMap(layer2);
		TexturePtr normalMap3 = _getNormalMap(layer3);

		float uvScale0 = mLayer[layer0].scale;
		float uvScale1 = mLayer[layer1].scale;
		float uvScale2 = mLayer[layer2].scale;
		float uvScale3 = mLayer[layer3].scale;

		SamplerState state;
		state.Address = TEXA_CLAMP;

		if (techId > 0)
			render->SetTexture(0, state, weightMap.c_ptr());

		state.Address = TEXA_WRAP;
		render->SetTexture(1, state, detailMap0.c_ptr());
		render->SetTexture(5, state, normalMap0.c_ptr());

		if (techId > 0)
		{
			render->SetTexture(2, state, detailMap1.c_ptr());
			render->SetTexture(6, state, normalMap1.c_ptr());
		}

		if (techId > 1)
		{
			render->SetTexture(3, state, detailMap2.c_ptr());
			render->SetTexture(7, state, normalMap2.c_ptr());
		}

		if (techId > 2)
		{
			render->SetTexture(4, state, detailMap3.c_ptr());
			render->SetTexture(8, state, normalMap3.c_ptr());
		}

		uTransform[techId]->SetUnifom(xOff, 0, zOff, 0);
		uUVParam[techId]->SetUnifom(xInvSectionSize, zInvSectionSize, xInvSize, zInvSize);
		uUVScale[techId]->SetUnifom(uvScale0, uvScale1, uvScale2, uvScale3);

		section->PreRender();

		render->Render(mTech[techId], &section->mRender);
	}
}

int	Terrain::_getTechId(int layer0, int layer1, int layer2, int layer3)
{
	int techId = kMaxDetailLevel - 1;

	if (layer3 >= 0)
		return techId;
	else if (layer2 >= 0)
		return techId - 1;
	else if (layer1 >= 0)
		return techId - 2;
	else
		return techId - 3;
}

void Terrain::RenderInMirror()
{
	RenderSystem * render = RenderSystem::Instance();

	ShaderParam * uTransform[kMaxDetailLevel];
	ShaderParam * uUVParam[kMaxDetailLevel];
	ShaderParam * uUVScale[kMaxDetailLevel];
	ShaderParam * uMorph[kMaxDetailLevel];

	for (int i = 0; i < kMaxDetailLevel; ++i)
	{
		uTransform[i] = mTechMirror[i]->GetVertexShaderParamTable()->GetParam("gTransform");
		uUVParam[i] = mTechMirror[i]->GetVertexShaderParamTable()->GetParam("gUVParam");
		uUVScale[i] = mTechMirror[i]->GetVertexShaderParamTable()->GetParam("gUVScale");
		uMorph[i] = mTechMirror[i]->GetVertexShaderParamTable()->GetParam("gMorph");
	}

	float xInvSectionSize = 1 / mConfig.xSectionSize;
	float zInvSectionSize = 1 / mConfig.zSectionSize;
	float xInvSize = 1 / mConfig.xSize;
	float zInvSize = 1 / mConfig.zSize;

	for (int i = 0; i < mVisibleSections.Size(); ++i)
	{
		TerrainSection * section = mVisibleSections[i];
		int x = section->GetSectionX();
		int z = section->GetSectionZ();
		int layer0 = section->GetLayer(0);
		int layer1 = section->GetLayer(1);
		int layer2 = section->GetLayer(2);
		int layer3 = section->GetLayer(3);
		float xOff = section->GetOffX();
		float zOff = section->GetOffZ();

		int techId = _getTechId(layer0, layer1, layer2, layer3);

		layer0 = Math::Maximum(0, layer0);
		layer1 = Math::Maximum(0, layer1);
		layer2 = Math::Maximum(0, layer2);
		layer3 = Math::Maximum(0, layer3);

		TexturePtr weightMap = GetWeightMap(x, z);
		TexturePtr detailMap0 = _getDetailMap(layer0);
		TexturePtr detailMap1 = _getDetailMap(layer1);
		TexturePtr detailMap2 = _getDetailMap(layer2);
		TexturePtr detailMap3 = _getDetailMap(layer3);

		float uvScale0 = mLayer[layer0].scale;
		float uvScale1 = mLayer[layer1].scale;
		float uvScale2 = mLayer[layer2].scale;
		float uvScale3 = mLayer[layer3].scale;

		SamplerState state;
		state.Address = TEXA_CLAMP;

		if (techId > 0)
			render->SetTexture(0, state, weightMap.c_ptr());

		state.Address = TEXA_WRAP;
		render->SetTexture(1, state, detailMap0.c_ptr());

		if (techId > 0)
		{
			render->SetTexture(2, state, detailMap1.c_ptr());
		}

		if (techId > 1)
		{
			render->SetTexture(3, state, detailMap2.c_ptr());
		}

		if (techId > 2)
		{
			render->SetTexture(4, state, detailMap3.c_ptr());
		}

		uTransform[techId]->SetUnifom(xOff, 0, zOff, 0);
		uUVParam[techId]->SetUnifom(xInvSectionSize, zInvSectionSize, xInvSize, zInvSize);
		uUVScale[techId]->SetUnifom(uvScale0, uvScale1, uvScale2, uvScale3);

		section->PreRender();

		render->Render(mTechMirror[techId], &section->mRender);
	}
}


Vec3 Terrain::GetPosition(const Ray & ray)
{
	Vec3 result(0, 0, 0);
	const int iMaxCount = 1000;

	int i = 0;
	Vec3 pos = ray.origin;
	float y = 0;

	if ((ray.origin.x < mBound.minimum.x || ray.origin.x > mBound.maximum.x) ||
		(ray.origin.y < mBound.minimum.y || ray.origin.y > mBound.maximum.y) ||
		(ray.origin.z < mBound.minimum.z || ray.origin.z > mBound.maximum.z))
	{
		RayIntersectionInfo info = ray.Intersection(mBound);

		if (!info.iterscetion)
			return result;

		pos = ray.origin + (info.distance + 0.1f) * ray.direction;
	}

	if (ray.direction == Vec3::UnitY)
	{
		y = GetHeight(pos.x, pos.z);
		if (pos.y <= y)
		{
			result = Vec3(pos.x, y, pos.z);
		}
	}
	else if (ray.direction == Vec3::NegUnitY)
	{
		y = GetHeight(pos.x, pos.z);
		if (pos.y >= y)
		{
			result = Vec3(pos.x, y, pos.z);
		}
	}
	else
	{
		while (pos.x > mBound.minimum.x && pos.x < mBound.maximum.x &&
			pos.z > mBound.minimum.z && pos.z < mBound.maximum.z &&
			i++ < iMaxCount)
		{
			y = GetHeight(pos.x, pos.z);
			if (pos.y <= y)
			{
				result = Vec3(pos.x, y, pos.z);
				break;
			}

			pos += ray.direction;
		}
	}

	return result;
}

float Terrain::GetHeight(float x, float z)
{
	float sx = 0, sz = mConfig.zSize;
	float ex = mConfig.zSize, ez = 0;

	float fx = (x - sx) / (ex - sx) * (mConfig.xVertexCount - 1);
	float fz = (z - sz) / (ez - sz) * (mConfig.zVertexCount - 1);

	int ix = (int) fx;
	int iz = (int) fz;

	d_assert(ix >= 0 && ix <= mConfig.xVertexCount - 1 &&
			 iz >= 0 && iz <= mConfig.zVertexCount - 1);

	float dx = fx - ix;
	float dz = fz - iz;

	int ix1 = ix + 1;
	int iz1 = iz + 1;

	ix1 = Math::Minimum(ix1, mConfig.xVertexCount - 1);
	iz1 = Math::Minimum(iz1, mConfig.zVertexCount - 1);

	float a = GetHeight(ix,  iz);
	float b = GetHeight(ix1, iz);
	float c = GetHeight(ix,  iz1);
	float d = GetHeight(ix1, iz1);
	float m = (b + c) * 0.5f;
	float h1, h2, final;

	if (dx + dz <= 1.0f)
	{
		d = m + (m - a);
	}
	else
	{
		a = m + (m - d);
	}

	h1 = a * (1.0f - dx) + b * dx;
	h2 = c * (1.0f - dx) + d * dx; 
	final = h1 * (1.0f - dz) + h2 * dz;

	return final;
}

Vec3 Terrain::GetNormal(float x, float z)
{
	float sx = 0, sz = mConfig.zSize;
	float ex = mConfig.zSize, ez = 0;

	float fx = (x - sx) / (ex - sx) * (mConfig.xVertexCount - 1);
	float fz = (z - sz) / (ez - sz) * (mConfig.zVertexCount - 1);

	int ix = (int) fx;
	int iz = (int) fz;

	d_assert(ix >= 0 && ix <= mConfig.xVertexCount - 1 &&
		iz >= 0 && iz <= mConfig.zVertexCount - 1);

	float dx = fx - ix;
	float dz = fz - iz;

	int ix1 = ix + 1;
	int iz1 = iz + 1;

	ix1 = Math::Minimum(ix1, mConfig.xVertexCount - 1);
	iz1 = Math::Minimum(iz1, mConfig.zVertexCount - 1);

	Color4 a = GetNormal(ix,  iz).ToColor4();
	Color4 b = GetNormal(ix1, iz).ToColor4();
	Color4 c = GetNormal(ix,  iz1).ToColor4();
	Color4 d = GetNormal(ix1, iz1).ToColor4();
	Color4 m = (b + c) * 0.5f;
	Color4 h1, h2, final;

	if (dx + dz <= 1.0f)
	{
		d = m + (m - a);
	}
	else
	{
		a = m + (m - d);
	}

	h1 = a * (1.0f - dx) + b * dx;
	h2 = c * (1.0f - dx) + d * dx; 
	final = h1 * (1.0f - dz) + h2 * dz;

	Vec3 normal;

	normal.x = final.r * 2 - 1;
	normal.y = final.g * 2 - 1;
	normal.z = final.b * 2 - 1;

	return normal;
}

float *	Terrain::LockHeight(const Rect & rc)
{
	d_assert (mLockedData == NULL);

	int w = rc.x2 - rc.x1 + 1;
	int h = rc.y2 - rc.y1 + 1;

	d_assert (w > 0 && h > 0);

	mLockedData = new float[w * h];

	int index = 0;
	for (int j = rc.y1; j <= rc.y2; ++j)
	{
		for (int i = rc.x1; i <= rc.x2; ++i)
		{
			mLockedData[index++] = GetHeight(i, j);
		}
	}

	mLockedRect = rc;

	return mLockedData;
}

void Terrain::UnlockHeight()
{
	d_assert (mLockedData != NULL);

	mLockedRect.x1 = Math::Maximum(0, mLockedRect.x1);
	mLockedRect.x2 = Math::Maximum(0, mLockedRect.x2);
	mLockedRect.x2 = Math::Minimum(mConfig.xVertexCount - 1, mLockedRect.x2);
	mLockedRect.y2 = Math::Minimum(mConfig.zVertexCount - 1, mLockedRect.y2);

	int index = 0;
	for (int j = mLockedRect.y1; j <= mLockedRect.y2; ++j)
	{
		for (int i = mLockedRect.x1; i <= mLockedRect.x2; ++i)
		{
			mHeights[j * mConfig.xVertexCount + i] = mLockedData[index++];
		}
	}

	// need re-calculate normals
	Rect rcNormal = mLockedRect;
	rcNormal.x1 -= 2;
	rcNormal.x2 += 2;
	rcNormal.y1 -= 2;
	rcNormal.y2 += 2;

	rcNormal.x1 = Math::Maximum(0, rcNormal.x1);
	rcNormal.y1 = Math::Maximum(0, rcNormal.y1);
	rcNormal.x2 = Math::Minimum(mConfig.xVertexCount - 1, rcNormal.x2);
	rcNormal.y2 = Math::Minimum(mConfig.zVertexCount - 1, rcNormal.y2);

	for (int j = rcNormal.y1; j < rcNormal.y2; ++j)
	{
		for (int i = rcNormal.x1; i < rcNormal.x2; ++i)
		{
			Vec3 a = _getPosition(i - 1, j + 0);
			Vec3 b = _getPosition(i + 0, j - 1);
			Vec3 c = _getPosition(i + 1, j + 0);
			Vec3 d = _getPosition(i + 0, j + 1);
			Vec3 p = _getPosition(i + 0, j + 0);

			Vec3 L = a - p, T = b - p, R = c - p, B = d - p;

			Vec3 N = Vec3::Zero;
			float len_L = L.Length(), len_T = T.Length();
			float len_R = R.Length(), len_B = B.Length();

			if (len_L > 0.01f && len_T > 0.01f)
				N += L.CrossN(T);

			if (len_T > 0.01f && len_R > 0.01f)
				N += T.CrossN(R);

			if (len_R > 0.01f && len_B > 0.01f)
				N += R.CrossN(B);

			if (len_B > 0.01f && len_L > 0.01f)
				N += B.CrossN(L);

			N.NormalizeL();
			N = (N + Vec3::Unit) / 2 * 255.0f;

			unsigned char cr = (unsigned char)N.x;
			unsigned char cg = (unsigned char)N.y;
			unsigned char cb = (unsigned char)N.z;
			unsigned char ca = 255;

			mNormals[j * mConfig.xVertexCount + i] = Color(cr, cg, cb, ca);
		}
	}

	// update sections
	for (int i = 0; i < mSections.Size(); ++i)
	{
		mSections[i]->NotifyUnlockHeight();
	}

	// update bound
	index = 0;
	for (int j = mLockedRect.y1; j <= mLockedRect.y2; ++j)
	{
		for (int i = mLockedRect.x1; i <= mLockedRect.x2; ++i)
		{
			float h = mLockedData[index++];
			mBound.minimum.y = Math::Minimum(mBound.minimum.y, h);
			mBound.maximum.y = Math::Maximum(mBound.maximum.y, h);
		}
	}

	safe_delete_array(mLockedData);
}

float * Terrain::LockWeightMap(const Rect & rc)
{
	d_assert (!IsLockedWeightMap());

	int w = rc.x2 - rc.x1 + 1;
	int h = rc.y2 - rc.y1 + 1;

	d_assert (w > 0 && h > 0);

	mLockedWeightMapData = new float[w * h];

	int index = 0;
	for (int j = rc.y1; j <= rc.y2; ++j)
	{
		for (int i = rc.x1; i <= rc.x2; ++i)
		{
			mLockedWeightMapData[index++] = 0;
		}
	}

	mLockedWeightMapRect = rc;

	return mLockedWeightMapData;
}

void Terrain::UnlockWeightMap(int layer)
{
	d_assert (IsLockedWeightMap());

	int index = 0;
	for (int j = mLockedWeightMapRect.y1; j <= mLockedWeightMapRect.y2; ++j)
	{
		for (int i = mLockedWeightMapRect.x1; i <= mLockedWeightMapRect.x2; ++i)
		{
			float weight = mLockedWeightMapData[index++];
			int xSection = i / kWeightMapSize;
			int zSection = j / kWeightMapSize;

			TerrainSection * section = GetSection(xSection, zSection);

			int layer0 = section->GetLayer(0);
			int layer1 = section->GetLayer(1);
			int layer2 = section->GetLayer(2);
			int layer3 = section->GetLayer(3);

			Color c = mWeights[j * mConfig.xWeightMapSize + i];
			Color4 c4;

			c4.a = c.a / 255.0f;
			c4.r = c.r / 255.0f;
			c4.g = c.g / 255.0f;
			c4.b = c.b / 255.0f;

			if (layer == layer0)
				c4.a += weight;
			else if (layer == layer1)
				c4.r += weight;
			else if (layer == layer2)
				c4.g += weight;
			else if (layer == layer3)
				c4.b += weight;
			else
			{
				if (layer0 == -1)
				{
					c4.a += weight;
					section->SetLayer(0, layer);
				}
				else if (layer1 == -1)
				{
					c4.r += weight;
					section->SetLayer(1, layer);
				}
				else if (layer2 == -1)
				{
					c4.g += weight;
					section->SetLayer(2, layer);
				}
				else if (layer3 == -1)
				{
					c4.b += weight;
					section->SetLayer(3, layer);
				}
			}

			c4 = c4.Normalize();

			c.r = unsigned char(c4.r * 255);
			c.g = unsigned char(c4.g * 255);
			c.b = unsigned char(c4.b * 255);
			c.a = unsigned char(c4.a * 255);

			mWeights[j * mConfig.xWeightMapSize + i] = c;
		}
	}

	// update weight map
	for (int i = 0; i < mSections.Size(); ++i)
	{
		TerrainSection * section = mSections[i];

		int xtile = Terrain::kWeightMapSize;
		int ztile = Terrain::kWeightMapSize;
		int x = section->GetSectionX();
		int z = section->GetSectionZ();

		Rect myRect;

		myRect.x1 = x * xtile;
		myRect.y1 = z * ztile;
		myRect.x2 = x * xtile + xtile;
		myRect.y2 = z * ztile + ztile;

		if (mLockedWeightMapRect.x1 > myRect.x2 || mLockedWeightMapRect.x2 < myRect.x1 ||
			mLockedWeightMapRect.y1 > myRect.y2 || mLockedWeightMapRect.y2 < myRect.y1)
			continue ;

		TexturePtr weightMap = GetWeightMap(x, z);

		LockedBox lb;
		weightMap->Lock(0, &lb, NULL, LOCK_DISCARD);

		Color * data = mWeights + z * kWeightMapSize * mConfig.xWeightMapSize + x * kWeightMapSize;
		int * dest = (int *)lb.pData;
		for (int k = 0; k < kWeightMapSize; ++k)
		{
			for (int p = 0; p < kWeightMapSize; ++p)
				dest[p] = M_RGBA(data[p].r, data[p].g, data[p].b, data[p].a);

			dest += kWeightMapSize;
			data += mConfig.xWeightMapSize;
		}

		weightMap->Unlock(0);
	}

	safe_delete_array(mLockedWeightMapData);
}


//bool Terrain::RayTrace(PhyHitInfo & info, const Ray & ray, float dist)
//{
//	const int iMaxCount = 1000;
//
//	int i = 0;
//	Vec3 pos = ray.origin;
//	float y = 0;
//
//	if ((ray.origin.x < mBound.minimum.x || ray.origin.x > mBound.maximum.x) ||
//		(ray.origin.y < mBound.minimum.y || ray.origin.y > mBound.maximum.y) ||
//		(ray.origin.z < mBound.minimum.z || ray.origin.z > mBound.maximum.z))
//	{
//		RayIntersectionInfo info = ray.Intersection(mBound);
//
//		if (!info.iterscetion)
//			return false;
//
//		pos = ray.origin + (info.distance + 0.1f) * ray.direction;
//	}
//
//	if (ray.direction == Vec3::UnitY)
//	{
//		y = GetHeight(pos.x, pos.z);
//		if (pos.y <= y && y - ray.origin.y < dist)
//		{
//			info.Hitted = true;
//			info.node = NULL;
//			info.Distance = y - ray.origin.y;
//			info.Normal = GetNormal(pos.x, pos.z);
//			info.MaterialId = -1;
//			return true;
//		}
//	}
//	else if (ray.direction == Vec3::NegUnitY)
//	{
//		y = GetHeight(pos.x, pos.z);
//		if (pos.y >= y && ray.origin.y - y < dist)
//		{
//			info.Hitted = true;
//			info.node = NULL;
//			info.Distance = ray.origin.y - y;
//			info.Normal = GetNormal(pos.x, pos.z);
//			info.MaterialId = -1;
//			return true;
//		}
//	}
//	else
//	{
//		while (pos.x > mBound.minimum.x && pos.x < mBound.maximum.x &&
//			   pos.z > mBound.minimum.z && pos.z < mBound.maximum.z &&
//			i++ < iMaxCount)
//		{
//			if (pos.Distance(ray.origin) >= dist)
//				return false;
//
//			y = GetHeight(pos.x, pos.z);
//			float d = ray.origin.Distance(Vec3(pos.x, y, pos.z));
//
//			if (pos.y <= y && d < dist)
//			{
//				info.Hitted = true;
//				info.node = NULL;
//				info.Distance = d;
//				info.Normal = GetNormal(pos.x, pos.z);
//				info.MaterialId = -1;
//
//				return true;
//			}
//
//			pos += ray.direction;
//		}
//	}
//
//	return false;
//}



}