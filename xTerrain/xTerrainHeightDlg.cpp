#include "stdafx.h"
#include "xTerrainHeightDlg.h"
#include "xTerrainPane.h"
#include "xApp.h"
#include "resource.h"

//////////////////////////////////////////////////////////////////////////
//
// Terrain Height
//

BEGIN_MESSAGE_MAP(xTerrainHeightDlg, CDialog)

	ON_CBN_SELCHANGE(IDC_Combo_TH_BrushOp, OnOpChanged)
	ON_CBN_SELCHANGE(IDC_Combo_TH_BrushType, OnTypeChanged)
	ON_WM_HSCROLL()

	ON_EN_CHANGE(IDC_Edit_TH_BrushSize, OnBrushSize)
	ON_EN_CHANGE(IDC_Edit_TH_BrushDensity, OnBrushDensity)

END_MESSAGE_MAP()

xTerrainHeightDlg::xTerrainHeightDlg()
{
}

xTerrainHeightDlg::~xTerrainHeightDlg()
{
}

BOOL xTerrainHeightDlg::OnInitDialog()
{
	if (!CDialog::OnInitDialog())
		return FALSE;

	return TRUE;
}

void xTerrainHeightDlg::_Init(Event * sender)
{
	CComboBox * cbBrushType = (CComboBox *)GetDlgItem(IDC_Combo_TH_BrushType);
	CComboBox * cbBrushOp = (CComboBox *)GetDlgItem(IDC_Combo_TH_BrushOp);

	cbBrushOp->InsertString(0, "Up");
	cbBrushOp->InsertString(1, "Down");
	cbBrushOp->InsertString(2, "Smooth");
	cbBrushOp->SetCurSel(0);

	FileSystem fs("../Core/Terrain/Brushes");

	fs.Load();

	Archive::FileInfoVisitor v = fs.GetFileInfos();

	while (!v.Endof())
	{
		const Archive::FileInfo & info = v.Cursor()->second;

		if (info.type == Archive::FILE_ARCHIVE)
		{
			TString128 base = File::RemoveExternName(info.name);
			TString128 ext = File::GetExternName(info.name);

			if (ext == "png")
				cbBrushType->AddString(base.c_str());
		}

		++v;
	}

	cbBrushType->SetCurSel(0);

	OnOpChanged();
	OnTypeChanged();

	SetDlgItemText(IDC_Edit_TH_BrushSize, "50");
	SetDlgItemText(IDC_Edit_TH_BrushDensity, "1");
}

void xTerrainHeightDlg::OnOpChanged()
{
	CString str;

	GetDlgItemText(IDC_Combo_TH_BrushOp, str);

	xEditTerrainHeight * editTH = xTerrainPane::Instance()->GetTerrainHeight();

	xEditTerrainHeight::Op op = xEditTerrainHeight::eUp;

	if (str == "Up")
		op = xEditTerrainHeight::eUp;
	else if (str == "Down")
		op = xEditTerrainHeight::eDown;
	else if (str == "Smooth")
		op = xEditTerrainHeight::eSmooth;
	else
		op = xEditTerrainHeight::eNone;

	editTH->SetOperator(op);
}

void xTerrainHeightDlg::OnTypeChanged()
{
	CString str;

	GetDlgItemText(IDC_Combo_TH_BrushType, str);

	xEditTerrainHeight * editTH = xTerrainPane::Instance()->GetTerrainHeight();

	str = "brushes\\" + str + ".png";

	editTH->SetBrush((LPCTSTR)str);
}

void xTerrainHeightDlg::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar)
{
	CDialog::OnHScroll(nSBCode, nPos, pScrollBar);
}


void xTerrainHeightDlg::OnBrushSize()
{
	CString strSize;

	GetDlgItemText(IDC_Edit_TH_BrushSize, strSize);

	float size = (float)atof((LPCTSTR)strSize);

	if (size > 0 && size < 1000)
		xTerrainPane::Instance()->GetTerrainHeight()->SetBrushSize(size);
}

void xTerrainHeightDlg::OnBrushDensity()
{
	CString strDensity;

	GetDlgItemText(IDC_Edit_TH_BrushDensity, strDensity);

	float density = (float)atof((LPCTSTR)strDensity);

	if (density > 0 && density < 100)
		xTerrainPane::Instance()->GetTerrainHeight()->SetBrushDensity(density);
}