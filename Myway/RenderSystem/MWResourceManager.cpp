#include "MWResourceManager.h"

using namespace Myway;


ResourceGroup::ResourceGroup()
{
}

ResourceGroup::~ResourceGroup()
{
    Shudown();
}

void ResourceGroup::Init()
{
    ArchiveList::Iterator whr = mArchives.Begin();
    ArchiveList::Iterator end = mArchives.End();

    while (whr != end)
    {
        (*whr)->Load();

        ++whr;
    }
}

void ResourceGroup::Shudown()
{
    ArchiveList::Iterator whr = mArchives.Begin();
    ArchiveList::Iterator end = mArchives.End();

    while (whr != end)
    {
        (*whr)->Unload();
        delete *whr;

        ++whr;
    }

    mArchives.Clear();
}

bool ResourceGroup::Exist(const TString128 & name) const
{
    ArchiveList::ConstIterator whr = mArchives.Begin();
    ArchiveList::ConstIterator end = mArchives.End();

    while (whr != end && !(*whr)->Exist(name))
    {
        ++whr;
    }

    return whr != end ? TRUE : FALSE;
}

void ResourceGroup::AddArchive(Archive * archive)
{
    d_assert (GetArchive(archive->GetName()) == NULL);

    mArchives.PushBack(archive);
}

Archive * ResourceGroup::GetArchive(const TString128 & name)
{
    ArchiveList::Iterator whr = mArchives.Begin();
    ArchiveList::Iterator end = mArchives.End();

    while (whr != end && (*whr)->GetName() != name)
    {
        ++whr;
    }

    return whr != end ? *whr : NULL;
}

ResourceGroup::ArchiveVisitor ResourceGroup::GetArchives() const
{
    return ArchiveVisitor(mArchives.Begin(), mArchives.End());
}

int ResourceGroup::GetArchiveSize() const
{
    return mArchives.Size();
}

const Archive::FileInfo * ResourceGroup::GetFileInfo(const TString128 & name) const
{
    ArchiveList::ConstIterator whr = mArchives.Begin();
    ArchiveList::ConstIterator end = mArchives.End();

    const Archive::FileInfo * info = NULL;

    while (whr != end && !info)
    {
        info = (*whr)->GetFileInfo(name);
        ++whr;
    }

    return info;
}

void ResourceGroup::GetFileInfosByFloder(Archive::FileInfoList & list, const TString128 & floder) const
{
	ArchiveList::ConstIterator whr = mArchives.Begin();
	ArchiveList::ConstIterator end = mArchives.End();

	while (whr != end)
	{
		(*whr)->GetFileInfosByFloder(list, floder);
		++whr;
	}
}

void ResourceGroup::GetFileInfosByKey(Archive::FileInfoList & list, const TString128 & key) const
{
    ArchiveList::ConstIterator whr = mArchives.Begin();
    ArchiveList::ConstIterator end = mArchives.End();

    while (whr != end)
    {
        (*whr)->GetFileInfosByKey(list, key);
        ++whr;
    }
}


void ResourceGroup::RemoveArchive(const TString128 & name)
{
    ArchiveList::Iterator whr = mArchives.Begin();
    ArchiveList::Iterator end = mArchives.End();

    while (whr != end && (*whr)->GetName() != name)
    {
        ++whr;
    }

    if (whr != end)
    {
        mArchives.Erase(whr);
    }
}

void ResourceGroup::RemoveArchive(Archive * archive)
{
    RemoveArchive(archive->GetName());
}

DataStreamPtr ResourceGroup::Open(const TString128 & name)
{
    ArchiveList::Iterator whr = mArchives.Begin();
    ArchiveList::Iterator end = mArchives.End();

    while (whr != end && !(*whr)->Exist(name))
    {
        ++whr;
    }

	if (whr == end)
	{
		LOG_PRINT_FORMAT("Source: %s not exit\r\n", name.c_str());
		return NULL;
	}

    return (*whr)->Open(name);
}






IMP_SLN(ResourceManager);
ResourceManager::ResourceManager()
{
    INIT_SLN;
    mResourceLoader = new ResourceLoader();
}

ResourceManager::~ResourceManager()
{
    SHUT_SLN;
    Shutdown();
    
    safe_delete(mResourceLoader);
}

void ResourceManager::Init()
{
	mResourceGroup.Init();
}

void ResourceManager::Shutdown()
{
    FactoryMap::Iterator fwhr = mFactorys.Begin();
    FactoryMap::Iterator fend = mFactorys.End();

    while (fwhr != fend)
    {
        delete fwhr->second;
        ++fwhr;
    }

    mFactorys.Clear();
    mResourceGroup.Shudown();
}

void ResourceManager::AddArchiveFactory(ArchiveFactory * factory)
{
    mFactorys.Insert(factory->GetType(), factory);
}

void ResourceManager::RemoveArchiveFactory(const TString128 & type)
{
    FactoryMap::Iterator whr = mFactorys.Find(type);
    FactoryMap::Iterator end = mFactorys.End();

    if (whr != end)
    {
        delete whr->second;
        mFactorys.Erase(whr);
    }
}

Archive * ResourceManager::AddArchive(const TString128 & name, const TString128 & type)
{
    FactoryMap::Iterator fwhr = mFactorys.Find(type);
    FactoryMap::Iterator fend = mFactorys.End();

    Archive * archive = fwhr->second->CreateInstance(name);

    mResourceGroup.AddArchive(archive);

	return archive;
}

bool ResourceManager::Exist(const TString128 & name) const
{
    return mResourceGroup.Exist(name);
}

const Archive::FileInfo * ResourceManager::GetFileInfo(const TString128 & name) const
{
	return mResourceGroup.GetFileInfo(name);
}

Archive * ResourceManager::GetArchive(const TString128 & name)
{
    return mResourceGroup.GetArchive(name);
}

void ResourceManager::GetFileInfosByFloder(Archive::FileInfoList & list, const TString128 & floder) const
{
	mResourceGroup.GetFileInfosByFloder(list, floder.LowerR());
}

void ResourceManager::GetFileInfosByKey(Archive::FileInfoList & list, const TString128 & key) const
{
	mResourceGroup.GetFileInfosByKey(list, key.LowerR());
}

DataStreamPtr ResourceManager::OpenResource(const char * source, bool _notInFileSystem)
{
	if (!_notInFileSystem)
		return mResourceGroup.Open(source);
	else
	{
		if (File::Exist(source))
			return DataStreamPtr(new FileStream(source));
		else
			return NULL;
	}
}

void ResourceManager::SetResourceLoader(ResourceLoader * loader)
{
    d_assert (loader);

    if (mResourceLoader != loader)
    {
        delete mResourceLoader;
        mResourceLoader = loader;
    }
}

ResourceLoader * ResourceManager::GetResourceLoader()
{
    return mResourceLoader;
}

