// Copyright © 2025 aka perchik71. All rights reserved.
// Contacts: <email:timencevaleksej@gmail.com>
// License: https://www.gnu.org/licenses/lgpl-3.0.html

#include <CKPE.Common.UIListView.h>
#include <CKPE.Common.UIVarCommon.h>
#include <CKPE.Common.Interface.h>
#include <CKPE.Utils.h>
#include <commctrl.h>
#include <vssym32.h>

constexpr auto UI_CONTROL_CONDITION_ID = 0xFA0;
constexpr auto SIZEBUF = 1024;

namespace CKPE
{
	namespace Common
	{
		namespace UI
		{
			namespace ListView
			{
				static CRECT rc, rc2;
				static Canvas canvas(nullptr);
				static char FileName[SIZEBUF], FileType[SIZEBUF];
				static OnCustomDrawHandler sCustomDrawHandler = nullptr;
				static OnCustomDrawPluginsHandler sCustomDrawPluginsHandler = nullptr;

				CKPE_COMMON_API HTHEME Initialize(HWND hWindow) noexcept(true)
				{
					SetWindowSubclass(hWindow, ListViewSubclass, 0, 0);

					ListView_SetTextColor(hWindow, GetThemeSysColor(ThemeColor::ThemeColor_Text_4));
					ListView_SetTextBkColor(hWindow, GetThemeSysColor(ThemeColor::ThemeColor_ListView_Color));
					ListView_SetBkColor(hWindow, GetThemeSysColor(ThemeColor::ThemeColor_ListView_Color));

					// Under Wine, visual styles cause comctl32 to use DrawThemeText which
					// ignores NM_CUSTOMDRAW colors. Opting out of theming forces the classic
					// GDI rendering path where clrText/clrTextBk are respected.
					if (CKPE_UserUseWine())
					{
						_CONSOLE("[Wine][ListView] Initialize hwnd=%p calling SetWindowTheme", (void*)hWindow);
						SetWindowTheme(hWindow, L"", L"");
					}

					return OpenThemeData(hWindow, VSCLASS_SCROLLBAR);
				}

				CKPE_COMMON_API LRESULT CALLBACK ListViewSubclass(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam,
					[[maybe_unused]] UINT_PTR uIdSubclass, [[maybe_unused]] DWORD_PTR dwRefData) noexcept(true)
				{
					// Under Wine, LVS_EX_DOUBLEBUFFER causes the offscreen DC to reinitialize
					// with black text and white background, defeating all color overrides and
					// breaking LVN_GETDISPINFO text delivery. Strip it from any style change.
					if (uMsg == LVM_SETEXTENDEDLISTVIEWSTYLE && CKPE_UserUseWine())
					{
						bool hadDoubleBuf = (wParam & LVS_EX_DOUBLEBUFFER) != 0;
						wParam &= ~static_cast<WPARAM>(LVS_EX_DOUBLEBUFFER);
						lParam &= ~static_cast<LPARAM>(LVS_EX_DOUBLEBUFFER);
						if (hadDoubleBuf)
							_CONSOLE("[Wine][ListView] stripped LVS_EX_DOUBLEBUFFER hwnd=%p", (void*)hWnd);
						return DefSubclassProc(hWnd, uMsg, wParam, lParam);
					}

					if ((uMsg == WM_SETFOCUS) || (uMsg == WM_KILLFOCUS))
					{
						InvalidateRect(hWnd, nullptr, TRUE);
						UpdateWindow(hWnd);
					}
					else if (uMsg == WM_PAINT)
					{
						// Paint border
						LRESULT result = DefSubclassProc(hWnd, uMsg, wParam, lParam);

						HDC hdc = GetWindowDC(hWnd);
						*(HDC*)(((std::uintptr_t)&canvas + 0x8)) = hdc;

						if (GetWindowRect(hWnd, (LPRECT)&rc))
						{
							rc.Offset(-rc.Left, -rc.Top);

							if (GetFocus() == hWnd)
								canvas.Frame(rc, GetThemeSysColor(ThemeColor::ThemeColor_Divider_Highlighter_Pressed));
							else
								canvas.Frame(rc, GetThemeSysColor(ThemeColor::ThemeColor_Divider_Highlighter_Gradient_End));

							rc.Inflate(-1, -1);
							canvas.Frame(rc, GetThemeSysColor(ThemeColor::ThemeColor_Divider_Color));
						}

						// scrollbox detected grip
						if (GetClientRect(hWnd, (LPRECT)&rc2))
						{
							if ((abs(rc2.Width - rc.Width) > 5) && (abs(rc2.Height - rc.Height) > 5))
							{
								rc.Left = rc.Width - GetSystemMetrics(SM_CXVSCROLL);
								rc.Top = rc.Height - GetSystemMetrics(SM_CYHSCROLL);
								rc.Width = GetSystemMetrics(SM_CXVSCROLL);
								rc.Height = GetSystemMetrics(SM_CYHSCROLL);

								canvas.Fill(rc, GetThemeSysColor(ThemeColor::ThemeColor_Default));
							}
						}

						ReleaseDC(hWnd, hdc);
						return result;
					}

					return DefSubclassProc(hWnd, uMsg, wParam, lParam);
				}

				CKPE_COMMON_API void OnCustomDrawItemPlugins(HWND hWindow, LPDRAWITEMSTRUCT lpDrawItem) noexcept(true)
				{
					// If there are no list view items, skip this message. 
					if (lpDrawItem->itemID == -1)
						return;

					RECT rc = lpDrawItem->rcItem;
					Canvas canvas(lpDrawItem->hDC);

					BOOL Selected = (lpDrawItem->itemState & ODS_SELECTED) == ODS_SELECTED;

					canvas.Fill(rc, GetThemeSysColor(ThemeColor::ThemeColor_ListView_Color));

					FileName[0] = '\0';
					FileType[0] = '\0';

					if (CKPE_UserUseWine())
					{
						// Under Wine, ListView_GetItemText (which dispatches LVN_GETDISPINFO via
						// LVM_GETITEMTEXT) does not work for LPSTR_TEXTCALLBACK items when called
						// from within WM_DRAWITEM — the nested dispatch silently returns empty text.
						// Work around: fetch lParam directly (no callback needed), then manually
						// send LVN_GETDISPINFOA to the parent so the game's handler fills in the text.
						LVITEMA lvParam = {};
						lvParam.mask = LVIF_PARAM;
						lvParam.iItem = lpDrawItem->itemID;
						SendMessage(lpDrawItem->hwndItem, LVM_GETITEMA, 0, (LPARAM)&lvParam);

						auto idFrom = static_cast<UINT_PTR>(GetDlgCtrlID(lpDrawItem->hwndItem));
						HWND hParent = GetParent(lpDrawItem->hwndItem);

						NMLVDISPINFOA di = {};
						di.hdr.hwndFrom = lpDrawItem->hwndItem;
						di.hdr.idFrom = idFrom;
						di.hdr.code = LVN_GETDISPINFOA;
						di.item.mask = LVIF_TEXT;
						di.item.iItem = lpDrawItem->itemID;
						di.item.iSubItem = 0;
						di.item.lParam = lvParam.lParam;
						di.item.pszText = FileName;
						di.item.cchTextMax = SIZEBUF;
						SendMessage(hParent, WM_NOTIFY, (WPARAM)idFrom, (LPARAM)&di);

						di.item.iSubItem = 1;
						di.item.pszText = FileType;
						di.item.cchTextMax = SIZEBUF;
						SendMessage(hParent, WM_NOTIFY, (WPARAM)idFrom, (LPARAM)&di);

						if (lpDrawItem->itemID == 0)
							_CONSOLE("[Wine][WM_DRAWITEM] item=0 filename=\"%s\" filetype=\"%s\" hwnd=%p lParam=%p",
								FileName, FileType, (void*)lpDrawItem->hwndItem, (void*)lvParam.lParam);
					}
					else
					{
						ListView_GetItemText(lpDrawItem->hwndItem, lpDrawItem->itemID, 0, FileName, SIZEBUF);
						ListView_GetItemText(lpDrawItem->hwndItem, lpDrawItem->itemID, 1, FileType, SIZEBUF);
					}

					if (sCustomDrawPluginsHandler)
						sCustomDrawPluginsHandler(hWindow, lpDrawItem, FileName, FileType);

					// CHECKBOX

					auto hImageList = ListView_GetImageList(lpDrawItem->hwndItem, LVSIL_SMALL);
					if (hImageList)
					{
						int cx, cy;
						ImageList_GetIconSize(hImageList, &cx, &cy);

						if ((rc.bottom - rc.top > cy) && (rc.right - rc.left > (cx + 8)))
						{
							cy = ((rc.bottom - rc.top) - cy) >> 1;

							LVITEMA lvi = { 0 };
							lvi.mask = LVIF_IMAGE;
							lvi.iItem = lpDrawItem->itemID;
							ListView_GetItem(lpDrawItem->hwndItem, &lvi);

							ImageList_Draw(hImageList, lvi.iImage, lpDrawItem->hDC, rc.left + 2, rc.top + cy, ILD_TRANSPARENT);
						}
					}

					// TEXT

					canvas.Font.Assign(*(ThemeData::GetSingleton()->ThemeFont));

					SetBkMode(lpDrawItem->hDC, TRANSPARENT);
					SetTextColor(lpDrawItem->hDC, GetThemeSysColor(ThemeColor::ThemeColor_Text_4));

					CRECT rcText;
					ListView_GetSubItemRect(lpDrawItem->hwndItem, lpDrawItem->itemID, 0, LVIR_LABEL, (LPRECT)&rcText);
					rcText.Inflate(-2, -2);
					rcText.Left += 2;

					canvas.TextRect(rcText, FileName, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

					ListView_GetSubItemRect(lpDrawItem->hwndItem, lpDrawItem->itemID, 1, LVIR_LABEL, (LPRECT)&rcText);
					rcText.Inflate(-2, -2);
					rcText.Left += 2;

					canvas.TextRect(rcText, FileType, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS);

					if (Selected)
						// blend 40%
						canvas.FillWithTransparent(rc, GetThemeSysColor(ThemeColor::ThemeColor_ListView_Owner_Selected), 40);
				}

				CKPE_COMMON_API LRESULT OnCustomDraw(HWND hWindow, LPNMLVCUSTOMDRAW lpListView) noexcept(true)
				{
					if (sCustomDrawHandler)
					{
						bool NeedReturned = false;
						auto Result = sCustomDrawHandler(hWindow, lpListView, NeedReturned);
						if (NeedReturned) return Result;
					}

					switch (lpListView->nmcd.dwDrawStage) 
					{
						//Before the paint cycle begins
					case CDDS_PREPAINT:
						//request notifications for individual listview items
						return CDRF_NOTIFYITEMDRAW;
						//Before an item is drawn
					case CDDS_ITEMPREPAINT:
						return CDRF_NOTIFYSUBITEMDRAW;
						//Before a subitem is drawn
					case CDDS_SUBITEM | CDDS_ITEMPREPAINT:
					{
						lpListView->clrTextBk = GetThemeSysColor(ThemeColor_ListView_Color);

						if (lpListView->nmcd.hdr.idFrom == UI_CONTROL_CONDITION_ID)
						{
							if (lpListView->iSubItem == 0 || lpListView->iSubItem == 5)
								lpListView->clrText = GetThemeSysColor(ThemeColor_Text_2);
							else
								lpListView->clrText = GetThemeSysColor(ThemeColor_Text_4);

							// Under Wine, CDRF_NEWFONT causes the struct colors to be ignored.
							// Set the DC text color directly so Wine uses it during item draw.
							if (CKPE_UserUseWine())
								SetTextColor(lpListView->nmcd.hdc, lpListView->clrText);
							return CDRF_NEWFONT;
						}

						lpListView->clrText = GetThemeSysColor(ThemeColor_Text_4);
						// Under Wine, CDRF_NEWFONT causes the struct colors to be ignored.
						// Set the DC text color directly so Wine uses it during item draw.
						if (CKPE_UserUseWine())
							SetTextColor(lpListView->nmcd.hdc, lpListView->clrText);
						return CDRF_NEWFONT;
					}
					default:
						return CDRF_DODEFAULT;
					}
				}

				CKPE_COMMON_API void InstallCustomDrawHandler(OnCustomDrawHandler handler) noexcept(true)
				{
					sCustomDrawHandler = handler;
				}

				CKPE_COMMON_API void InstallCustomDrawPluginsHandler(OnCustomDrawPluginsHandler handler) noexcept(true)
				{
					sCustomDrawPluginsHandler = handler;
				}
			}
		}
	}
}