#include "OggSplitterDS.h"
#include <stdio.h>
#include <atlbase.h>
#include <ocidl.h>
    
HRESULT COggSplitter::GetTypeInfoCount(UINT FAR* pctinfo)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo FAR* FAR* pptinfo)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::GetIDsOfNames(REFIID riid, OLECHAR FAR* FAR* rgszNames, UINT cNames,
									LCID lcid, DISPID FAR* rgdispid)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::Invoke(DISPID dispidMember, REFIID riid, LCID lcid, WORD wFlags,
								  DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult,
									EXCEPINFO FAR* pexcepinfo, UINT FAR* puArgErr)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_AuthorName(BSTR FAR* pbstrAuthorName)
{
	*pbstrAuthorName = SysAllocString(L"TesT");
	return NOERROR;
}

HRESULT COggSplitter::get_Title(BSTR FAR* pbstrTitle)
{
	*pbstrTitle = SysAllocString(L"TestTitel");
	return NOERROR;
}

HRESULT COggSplitter::get_Rating(BSTR FAR* pbstrRating)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_Description(BSTR FAR* pbstrDescription)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_Copyright(BSTR FAR* pbstrCopyright)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_BaseURL(BSTR FAR* pbstrBaseURL)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_LogoURL(BSTR FAR* pbstrLogoURL)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_LogoIconURL(BSTR FAR* pbstrLogoURL)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_WatermarkURL(BSTR FAR* pbstrWatermarkURL)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_MoreInfoURL(BSTR FAR* pbstrMoreInfoURL)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_MoreInfoBannerImage(BSTR FAR* pbstrMoreInfoBannerImage)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_MoreInfoBannerURL(BSTR FAR* pbstrMoreInfoBannerURL)
{
	return E_NOTIMPL;
}

HRESULT COggSplitter::get_MoreInfoText(BSTR FAR* pbstrMoreInfoText)
{
	return E_NOTIMPL;
}
