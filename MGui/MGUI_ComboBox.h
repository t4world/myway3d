#pragma once

#include "MGUI_Widget.h"
#include "MGUI_ListBox.h"
#include "MGUI_Button.h"
#include "MGUI_EditBox.h"

namespace Myway {

	class MGUI_ENTRY MGUI_ComboBox : public MGUI_Widget
	{
		DeclareRTTI();

	public:
		tEvent1<int> eventSelectChanged;

	public:
		MGUI_ComboBox(const MGUI_LookFeel * _lookfeel, MGUI_Widget * _parent);
		virtual ~MGUI_ComboBox();

		void Append(const MGUI_String & _text, void * _userData = NULL);
		void Insert(int _index, const MGUI_String & _text, void * _userData = NULL);
		void Remove(int _index);
		void Clear();
		int GetCount();

		void SetSelectIndex(int _index);
		int GetSelectIndex() const;

		const MGUI_String & GetText(int _index);
		void * GetUserData(int _index);

		void SetItemHeight(int _height);
		int GetItemDY() const;

		virtual void OnUpdate();
		virtual void _AddRenderItem(MGUI_Layout * _layout);

	protected:
		void OnDrop_();
		void OnSelectChanged_(int _index);
		void OnInputMousePressed_(int _x, int _y, MGUI_MouseButton _button);

		void _drop(bool popuped);

	protected:
		MGUI_Button * mBnDrop;
		MGUI_EditBox * mEditBox;
		MGUI_ListBox * mListBox;

		bool mPopuped;
		int mItemHeight;
		int mDropHeight;

		tListener0<MGUI_ComboBox> OnDropButtonClick;
		tListener0<MGUI_ComboBox> OnEditBoxClick;
		tListener1<MGUI_ComboBox, int> OnListBoxSelectChanged;
		tListener3<MGUI_ComboBox, int, int, MGUI_MouseButton> OnInputMousePressed;
	};
}