//////////////////////////////////////////////////////////////////////////
//
// Infinite. Scene Editor For Myway3D
//   
//	 author: Myway
//	 e-mail: Myway3D@Gmail.com
//
//
//
#pragma once

#include "Shape.h"
#include "xEvent.h"
#include "xScene.h"
#include "xForest.h"
#include "xEnvironment.h"
#include "xRenderSetting.h"
#include "Gizmo.h"
#include "xUndoRedo.h"
#include "xObjBound.h"

#include "Common\\ColourPanel.h"
#include "Common\\FileDialog.h"
#include "MessageBox.h"
#include "Editor.h"
#include "Plugin.h"
#include "PluginDialog.h"

namespace Infinite {

	enum eOperator
	{
		eOP_Unknown,
		eOP_Pick,
		eOP_Move,
		eOP_Rotate,
		eOP_Scale,
		eOP_Terrain,

		MW_ALIGN_ENUM(eOperator)
	};

	enum ePick
	{
		PICK_Flag = 0x01,
	};

	class INFI_ENTRY Editor
	{
		DECLARE_SINGLETON(Editor);

	public:
		Editor();
		~Editor();

		void Init();
		void Shutdown();
		void Update();

		ShaderLib * GetHelperShaderLib() { return mHelperShaderLib; }

		void SetSelectedShape(Shape * obj);
		void SetSelectedShapes(Shape ** objs, int size);
		int GetSelectedShapeSize();
		Shape * GetSelectedShape(int index);

		Vec3 GetHitPosition(float fx, float fy);

		void SetOperator(eOperator op);
		eOperator GetOperator() { return mOperator; }

		void _SetMousePosition(const Point2f & pt) { mMousePosition = pt; }
		Point2f GetMousePosition() { return mMousePosition; }

		void SetFoucs(bool b) { mFoucs = b; }
		bool IsFoucs() { return mFoucs; }

		void AddPlugin(iPlugin * plugin) { mPlugins.PushBack(plugin); }
		int GetPluginCount() { return mPlugins.Size(); }
		iPlugin * GetPlugin(int i) { return mPlugins[i]; }

		int MessageBox(const char * text, const char * caption, UINT type);
		
		ColourPanel * getColorPanel();
		FileDialog * getFileDialog();
		MMessageBox * getMessageBox();
		PluginDialog * getPluginDialog();

		void SetGameMode(bool b);
		bool GetGameMode() { return mGameMode; }

	protected:
		void _loadPlugin();

	protected:
		void _unloadScene(Event * sender);

	protected:
		tEventListener<Editor> OnUnloadScene;

		ShaderLib * mHelperShaderLib;

		Array<Shape*> mSelectedShapes;
		ShapeManager mShapeMgr;

		xScene mScene;
		xForest mForest;
		xEnvironment mEnvironment;
		xRenderSetting mRenderSetting;

		ColourPanel * mColorPanel;
		FileDialog * mFileDialog;
		MMessageBox * mMessageBox;
		PluginDialog * mPuginDialog;

		eOperator mOperator;

		Gizmo mGizmo;
		xUndoRedoManager mUndoRedoManager;
		xObjBound mObjBound;

		Point2f mMousePosition;
		bool mFoucs;

		Array<iPlugin *> mPlugins;

		bool mGameMode;
	};

}