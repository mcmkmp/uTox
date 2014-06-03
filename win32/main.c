LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

_Bool draw = 0;

float scale = 1.0;
_Bool connected = 0;

enum
{
    MENU_TEXTINPUT = 101,
    MENU_MESSAGES = 102,
};

static TRACKMOUSEEVENT tme = {sizeof(TRACKMOUSEEVENT), TME_LEAVE, 0, 0};
static _Bool mouse_tracked = 0;

static _Bool hidden;

static HBITMAP h;

void drawbitmap(int bm, int x, int y, int width, int height)
{
    if(bitmap[bm] != h)
    {
        SelectObject(hdcMem, bitmap[bm]);
        h = bitmap[bm];
    }

    BitBlt(hdc, x, y, width, height, hdcMem, 0, 0, SRCCOPY);
}

void drawbitmaptrans(int bm, int x, int y, int width, int height)
{
    if(bitmap[bm] != h)
    {
        SelectObject(hdcMem, bitmap[bm]);
        h = bitmap[bm];
    }

    MaskBlt(hdc, x, y, width, height, hdcMem, 0, 0, h, 0, 0, MAKEROP4(0x00AA0029, SRCCOPY));
    //BitBlt(hdc, x, y, width, height, hdcMem, 0, 0, SRCAND);
    //TransparentBlt(hdc, x, y, width, height, hdcMem, 0, 0, width, height, ~0);
}

void drawbitmapalpha(int bm, int x, int y, int width, int height)
{
    BLENDFUNCTION ftn = {
                .BlendOp = AC_SRC_OVER,
                .BlendFlags = 0,
                .SourceConstantAlpha = 0xFF,
                .AlphaFormat = AC_SRC_ALPHA
            };

    if(bitmap[bm] != h)
    {
        SelectObject(hdcMem, bitmap[bm]);
        h = bitmap[bm];
    }

    AlphaBlend(hdc, x, y, width, height, hdcMem, 0, 0, width, height, ftn);
}

void drawtextwidth(int x, int width, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x + width, y + 256};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS);
}

void drawtextrange(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x2, y + 256};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE | DT_END_ELLIPSIS);
}

void drawtextrangecut(int x, int x2, int y, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, x2, y + 256};
    DrawText(hdc, (char*)str, length, &r, DT_SINGLELINE);
}

int drawtextrect(int x, int y, int right, int bottom, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, right, bottom};
    return DrawText(hdc, (char*)str, length, &r, DT_WORDBREAK);
}

/*int drawtextrect2(int x, int y, int right, int bottom, uint8_t *str, uint16_t length)
{
    RECT r = {x, y, right, bottom};
    DrawText(hdc, (char*)str, length, &r, DT_WORDBREAK | DT_CALCRECT);
    return r.right - r.left;
}*/

void drawrect(int x, int y, int width, int height, uint32_t color)
{
    RECT r = {x, y, x + width, y + height};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawhline(int x, int y, int x2, uint32_t color)
{
    RECT r = {x, y, x2, y + 1};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void drawvline(int x, int y, int y2, uint32_t color)
{
    RECT r = {x, y, x + 1, y2};
    SetDCBrushColor(hdc, color);
    FillRect(hdc, &r, hdc_brush);
}

void fillrect(RECT *r, uint32_t color)
{
    SetDCBrushColor(hdc, color);
    FillRect(hdc, r, hdc_brush);
}

void framerect(RECT *r, uint32_t color)
{
    SetDCBrushColor(hdc, color);
    FrameRect(hdc, r, hdc_brush);
}

void setfont(int id)
{
    if(id == FONT_TEXT){SelectObject(hdc, font_small);}
    if(id == FONT_TEXT_LARGE){SelectObject(hdc, font_med2);}
    if(id == FONT_BUTTON){SelectObject(hdc, font_med2);}
    if(id == FONT_TITLE){SelectObject(hdc, font_big);}
    if(id == FONT_SUBTITLE){SelectObject(hdc, font_big2);}
    //SelectObject(hdc, font[id]);
}

void setcolor(uint32_t color)
{
    SetTextColor(hdc, color);
}

void setbkcolor(uint32_t color)
{
    SetBkColor(hdc, color);
}

void setbgcolor(uint32_t color)
{
    if(color == ~0)
    {
        SetBkMode(hdc, TRANSPARENT);
    }
    else
    {
        SetBkMode(hdc, OPAQUE);
        SetBkColor(hdc, color);
    }
}

RECT clip[16];

static int clipk;

void pushclip(int left, int top, int width, int height)
{
    int right = left + width, bottom = top + height;

    RECT *r = &clip[clipk++];
    r->left = left;
    r->top = top;
    r->right = right;
    r->bottom = bottom;

    HRGN rgn = CreateRectRgn(left, top, right, bottom);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);
}

void popclip(void)
{
    clipk--;
    if(!clipk)
    {
        SelectClipRgn(hdc, NULL);
        return;
    }

    RECT *r = &clip[clipk - 1];

    HRGN rgn = CreateRectRgn(r->left, r->top, r->right, r->bottom);
    SelectClipRgn (hdc, rgn);
    DeleteObject(rgn);
}

void enddraw(int x, int y, int width, int height)
{
    BitBlt(main_hdc, x, y, width, height, hdc, x, y, SRCCOPY);
}

void address_to_clipboard(void)
{
    #define size sizeof(self.id)

    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, size + 1);
    uint8_t *p = GlobalLock(hMem);
    memcpy(p, self.id, size + 1);
    p[size] = 0;
    GlobalUnlock(hMem);
    OpenClipboard(0);
    EmptyClipboard();
    SetClipboardData(CF_TEXT, hMem);
    CloseClipboard();

    #undef size
}

void editpopup(void)
{
    POINT p;
    GetCursorPos(&p);

    HMENU hMenu = CreatePopupMenu();
    if(hMenu) {
        _Bool emptysel = (edit_sel.length == 0);

        InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_CUT, "Cut");
        InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_COPY, "Copy");
        InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_PASTE, "Paste");///gray out if clipboard empty
        InsertMenu(hMenu, -1, MF_BYPOSITION | (emptysel ? MF_GRAYED : 0), EDIT_DELETE, "Delete");
        InsertMenu(hMenu, -1, MF_BYPOSITION, EDIT_SELECTALL, "Select All");

        SetForegroundWindow(hwnd);

        TrackPopupMenu(hMenu, TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void listpopup(uint8_t item)
{
    POINT p;
    GetCursorPos(&p);

    HMENU hMenu = CreatePopupMenu();
    if(hMenu) {
        switch(item)
        {
            case ITEM_SELF:
            {
                InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_NONE) ? MF_CHECKED : 0), TRAY_STATUS_AVAILABLE, "Available");
                InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_AWAY) ? MF_CHECKED : 0), TRAY_STATUS_AWAY, "Away");
                InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_BUSY) ? MF_CHECKED : 0), TRAY_STATUS_BUSY, "Busy");
                break;
            }

            case ITEM_FRIEND:
            {
                InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_DELETE, "Remove");
                break;
            }

            case ITEM_GROUP:
            {
                InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_DELETE, "Leave");
                break;
            }

            case ITEM_ADDFRIEND:
            {
                break;
            }

            case ITEM_FRIEND_ADD:
            {
                InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_ACCEPT, "Accept");
                InsertMenu(hMenu, -1, MF_BYPOSITION, LIST_DELETE, "Ignore");
                break;
            }
        }

        SetForegroundWindow(hwnd);

        TrackPopupMenu(hMenu, TPM_TOPALIGN, p.x, p.y, 0, hwnd, NULL);
        DestroyMenu(hMenu);
    }
}

void togglehide(void)
{
    if(hidden)
    {
        ShowWindow(hwnd, SW_RESTORE);
        SetForegroundWindow(hwnd);
        redraw();
        hidden = 0;
    }
    else
    {
        ShowWindow(hwnd, SW_HIDE);
        hidden = 1;
    }
}

void ShowContextMenu(void)
{
	POINT pt;
	GetCursorPos(&pt);
	HMENU hMenu = CreatePopupMenu();
	if(hMenu)
	{
	    InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_SHOWHIDE, hidden ? "Restore" : "Hide");

	    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

	    InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_NONE) ? MF_CHECKED : 0), TRAY_STATUS_AVAILABLE, "Available");
	    InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_AWAY) ? MF_CHECKED : 0), TRAY_STATUS_AWAY, "Away");
	    InsertMenu(hMenu, -1, MF_BYPOSITION | ((self.status == TOX_USERSTATUS_BUSY) ? MF_CHECKED : 0), TRAY_STATUS_BUSY, "Busy");

	    InsertMenu(hMenu, -1, MF_BYPOSITION | MF_SEPARATOR, 0, NULL);

		InsertMenu(hMenu, -1, MF_BYPOSITION, TRAY_EXIT, "Exit");

		// note:	must set window to the foreground or the
		//			menu won't disappear when it should
		SetForegroundWindow(hwnd);

		TrackPopupMenu(hMenu, TPM_BOTTOMALIGN, pt.x, pt.y, 0, hwnd, NULL);
		DestroyMenu(hMenu);
	}
}

LRESULT nc_hit(int x, int y)
{
    uint8_t row, col;

    row = 1;
    col = 1;

    if(x < BORDER)
    {
        col = 0;
    }
    else if(x >= width - BORDER)
    {
        col = 2;
    }

    if(y < CAPTION + BORDER)
    {
        if(y < BORDER)
        {
            row = 0;
        }
        else if(col == 1)
        {
            if(x >= LIST_X && x < LIST_X + ITEM_WIDTH)
            {
                return (y >= LIST_Y) ? HTCLIENT : HTCAPTION;
            }

            return inrect(x, y, width - 91, 1, 90, 26) ? HTCLIENT : HTCAPTION;
        }
    }
    else if(y >= height - BORDER)
    {
        row = 2;
    }

    if(x >= width - 10 && y >= height - 10)
    {
        return HTBOTTOMRIGHT;
    }

    const LRESULT result[3][3] =
    {
        {HTTOPLEFT, HTTOP, HTTOPRIGHT},
        {HTLEFT, HTCLIENT, HTRIGHT},
        {HTBOTTOMLEFT, HTBOTTOM, HTBOTTOMRIGHT},
    };

    return result[row][col];
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR lpCmdLine, int nCmdShow)
{
    MSG msg;
    int x, y;
    const char classname[] = "winTox";

    HICON myicon = LoadIcon(hInstance, MAKEINTRESOURCE(101));

    WNDCLASS wc = {
        .style = CS_OWNDC,
        .lpfnWndProc = WindowProc,
        .hInstance = hInstance,
        .hIcon = myicon,
        .lpszClassName = classname,
        .hCursor = LoadCursor(NULL, IDC_ARROW),
    };

    NOTIFYICONDATA nid = {
        .uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP,
        .uCallbackMessage = WM_NOTIFYICON,
        .hIcon = myicon,
        .szTip = "Tox - tooltip",
        .cbSize = sizeof(nid),
    };

    LOGFONT lf = {
        .lfWeight = FW_NORMAL,
        //.lfCharSet = ANSI_CHARSET,
        .lfOutPrecision = OUT_TT_PRECIS,
        .lfQuality = CLEARTYPE_QUALITY,
        .lfFaceName = "Arial",
    };

    BITMAP bm = {
        .bmWidth = 16,
        .bmHeight = 10,
        .bmWidthBytes = 2,
        .bmPlanes = 1,
        .bmBitsPixel = 1
    };

    BITMAP bm2 = {
        .bmWidth = 8,
        .bmHeight = 8,
        .bmWidthBytes = 32,
        .bmPlanes = 1,
        .bmBitsPixel = 32
    };

    //start tox thread
    thread(tox_thread, NULL);

    RegisterClass(&wc);

    x = (GetSystemMetrics(SM_CXSCREEN) - MAIN_WIDTH) / 2;
    y = (GetSystemMetrics(SM_CYSCREEN) - MAIN_HEIGHT) / 2;
    hwnd = CreateWindowEx(WS_EX_APPWINDOW, classname, "Tox", WS_POPUP | WS_MINIMIZEBOX | WS_MAXIMIZEBOX, x, y, MAIN_WIDTH, MAIN_HEIGHT, NULL, NULL, hInstance, NULL);

    hdc_brush = GetStockObject(DC_BRUSH);

    ShowWindow(hwnd, nCmdShow);

    tme.hwndTrack = hwnd;

    nid.hWnd = hwnd;
    Shell_NotifyIcon(NIM_ADD, &nid);

    lf.lfHeight = -20;
    font_big = CreateFontIndirect(&lf);
    lf.lfHeight = -18;
    font_big2 = CreateFontIndirect(&lf);
    lf.lfHeight = -16;
    font_med = CreateFontIndirect(&lf);
    lf.lfHeight = -14;
    font_med2 = CreateFontIndirect(&lf);
    lf.lfHeight = -12;
    font_small = CreateFontIndirect(&lf);

    bm.bmBits = bm_minimize_bits;
    bitmap[BM_MINIMIZE] = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_restore_bits;
    bitmap[BM_RESTORE] = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_maximize_bits;
    bitmap[BM_MAXIMIZE] = CreateBitmapIndirect(&bm);
    bm.bmBits = bm_exit_bits;
    bitmap[BM_EXIT] = CreateBitmapIndirect(&bm);

    bm.bmBits = bm_plus_bits;
    bm.bmHeight = 16;
    bitmap[BM_PLUS] = CreateBitmapIndirect(&bm);
    //153, 182, 224

    bitmap[BM_ONLINE] = CreateBitmap(10, 10, 1, 32, bm_online_bits);
    bitmap[BM_AWAY] = CreateBitmap(10, 10, 1, 32, bm_away_bits);
    bitmap[BM_BUSY] = CreateBitmap(10, 10, 1, 32, bm_busy_bits);
    bitmap[BM_OFFLINE] = CreateBitmap(10, 10, 1, 32, bm_offline_bits);
    bitmap[BM_CONTACT] = CreateBitmap(48, 48, 1, 32, bm_contact_bits);
    bitmap[BM_GROUP] = CreateBitmap(48, 48, 1, 32, bm_group_bits);

    uint32_t test[64];
    int xx = 0;
    while(xx < 8)
    {
        int y = 0;
        while(y < 8)
        {
            uint32_t value = 0xFFFFFF;
            if(xx + y >= 7)
            {
                int a = xx % 3, b = y % 3;
                if(a == 1)
                {
                    if(b == 0)
                    {
                        value = 0xB6B6B6;
                    }
                    else if(b == 1)
                    {
                        value = 0x999999;
                    }
                }
                else if(a == 0 && b == 1)
                {
                    value = 0xE0E0E0;
                }
            }

            test[y * 8 + xx] = value;
            y++;
        }
        xx++;
    }

    bm2.bmBits = test;
    bitmap[BM_CORNER] = CreateBitmapIndirect(&bm2);

    TEXTMETRIC tm;
    SelectObject(hdc, font_small);
    GetTextMetrics(hdc, &tm);

    font_small_lineheight = tm.tmHeight + tm.tmExternalLeading;


    //wait for tox_thread init
    while(!tox_thread_run) {Sleep(1);}

    list_start();
    draw = 1;
    redraw();

    while(GetMessage(&msg, NULL, 0, 0))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    tox_thread_run = 0;

    //cleanup

    //delete tray icon
    Shell_NotifyIcon(NIM_DELETE, &nid);



    //wait for tox_thread cleanup
    while(!tox_thread_run) {Sleep(1);}


    printf("exit\n");

    return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch(msg)
    {
        case WM_DESTROY:
        {
            PostQuitMessage(0);
            return 0;
        }

        case WM_GETMINMAXINFO:
        {
            POINT min = {800, 320};
            ((MINMAXINFO*)lParam)->ptMinTrackSize = min;

            break;
        }

        case WM_CREATE:
        {
            main_hdc = GetDC(hwnd);
            hdc = CreateCompatibleDC(main_hdc);
            hdcMem = CreateCompatibleDC(hdc);

            return 0;
        }

        case WM_SIZE:
        {
            switch(wParam)
            {
                case SIZE_MAXIMIZED:
                {
                    maximized = 1;
                    break;
                }

                case SIZE_RESTORED:
                {
                    maximized = 0;
                    break;
                }
            }

            int w, h;

            w = GET_X_LPARAM(lParam);
            h = GET_Y_LPARAM(lParam);

            if(w != 0)
            {
                width = w;
                height = h;

                if(hdc_bm)
                {
                    DeleteObject(hdc_bm);
                }

                hdc_bm = CreateCompatibleBitmap(main_hdc, width, height);
                SelectObject(hdc, hdc_bm);
                redraw();
            }
            break;
        }

        case WM_ERASEBKGND:
        {
            return 1;
        }

        case WM_PAINT:
        {
            PAINTSTRUCT ps;

            BeginPaint(hwnd, &ps);

            RECT r = ps.rcPaint;
            BitBlt(main_hdc, r.left, r.top, r.right - r.left, r.bottom - r.top, hdc, r.left, r.top, SRCCOPY);

            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_NCHITTEST:
        {
            POINT p = {GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam)};

            ScreenToClient(hwnd, &p);

            return nc_hit(p.x, p.y);
        }

        case WM_KEYDOWN:
        {
            switch(wParam)
            {
                case VK_F1:
                {
                    FRIEND *f = sitem->data;
                    uint64_t *data = malloc(8 + 12);
                    data[0] = 2451227;
                    memcpy(&data[1], "libtoxav.dll", 12);

                    tox_postmessage(TOX_SENDFILE, f - friend, 12, data);
                    break;
                }

                case VK_ESCAPE:
                {
                    edit_resetfocus();
                    return 0;
                }

                case VK_DELETE:
                {
                    if(edit_active())
                    {
                        edit_delete();
                        break;
                    }

                    list_deletesitem();
                    return 0;
                }
            }

            if(GetKeyState(VK_CONTROL) & 0x80)
            {
                switch(wParam)
                {
                    case 'C':
                    {
                        if(edit_active())
                        {
                            edit_copy();
                            break;
                        }

                        if(sitem)
                        {
                            if(sitem->item == ITEM_FRIEND)
                            {
                                FRIEND *f = sitem->data;
                                messages_copy(f->message);
                            }
                            else if(sitem->item == ITEM_GROUP)
                            {
                                GROUPCHAT *g = sitem->data;
                                messages_copy(g->message);
                            }
                        }

                        break;
                    }

                    case 'V':
                    {
                        if(edit_active())
                        {
                            edit_paste();
                            break;
                        }
                        break;
                    }

                    case 'A':
                    {
                        if(edit_active())
                        {
                            edit_selectall();
                            break;
                        }
                        break;
                    }
                }
            }

            break;
        }

        case WM_CHAR:
        {
            if(edit_active())
            {
                edit_char(wParam);
                return 0;
            }

            return 0;
        }

        case WM_MOUSEWHEEL:
        {
            panel_mwheel(&panel_main, 0, 0, width, height, (double)((int16_t)HIWORD(wParam)) / (double)(WHEEL_DELTA));
            return 0;
        }

        case WM_MOUSEMOVE:
        {
            static int my;
            int x, y, dy;

            x = GET_X_LPARAM(lParam);
            y = GET_Y_LPARAM(lParam);

            dy = y - my;
            my = y;

            panel_mmove(&panel_main, 0, 0, width, height, x, y, dy);

            if(!mouse_tracked)
            {
                TrackMouseEvent(&tme);
                mouse_tracked = 1;
            }

            break;
        }

        case WM_LBUTTONDOWN:
        {
            panel_mdown(&panel_main);
            SetCapture(hwnd);
            break;
        }

        case WM_RBUTTONDOWN:
        {
            panel_mright(&panel_main);
            break;
        }

        case WM_RBUTTONUP:
        {
            break;
        }

        case WM_LBUTTONUP:
        {
            panel_mup(&panel_main);
            ReleaseCapture();
            break;
        }

        case WM_MOUSELEAVE:
        {
            panel_mleave(&panel_main);
            mouse_tracked = 0;
            break;
        }


        case WM_COMMAND:
        {
            int menu = LOWORD(wParam);//, msg = HIWORD(wParam);

            switch(menu)
            {
                case TRAY_SHOWHIDE:
                {
                    togglehide();
                    break;
                }

                case TRAY_EXIT:
                {
                    PostQuitMessage(0);
                    break;
                }

                #define setstatus(x) if(self.status != x) { \
                    tox_postmessage(TOX_SETSTATUS, x, 0, NULL); self.status = x; redraw(); }

                case TRAY_STATUS_AVAILABLE:
                {
                    setstatus(TOX_USERSTATUS_NONE);
                    break;
                }

                case TRAY_STATUS_AWAY:
                {
                    setstatus(TOX_USERSTATUS_AWAY);
                    break;
                }

                case TRAY_STATUS_BUSY:
                {
                    setstatus(TOX_USERSTATUS_BUSY);
                    break;
                }


                case EDIT_CUT:
                {
                    edit_cut();
                    break;
                }

                case EDIT_COPY:
                {
                    edit_copy();
                    break;
                }

                case EDIT_PASTE:
                {
                    edit_paste();
                    break;
                }

                case EDIT_DELETE:
                {
                    edit_delete();
                    break;
                }

                case EDIT_SELECTALL:
                {
                    edit_selectall();
                    break;
                }

                case LIST_DELETE:
                {
                    list_deleteritem();
                    break;
                }

                case LIST_ACCEPT:
                {
                    FRIENDREQ *req = ritem->data;
                    tox_postmessage(TOX_ACCEPTFRIEND, 0, 0, req);
                    break;
                }
            }

            break;
        }

        case WM_NOTIFYICON:
        {
            int msg = LOWORD(lParam);

            switch(msg)
            {
                case WM_MOUSEMOVE:
                {
                    break;
                }

                case WM_LBUTTONDOWN:
                case WM_LBUTTONDBLCLK:
                {
                    togglehide();
                    break;
                }

                case WM_LBUTTONUP:
                {
                    break;
                }

                case WM_RBUTTONDOWN:
                {
                    break;
                }

                case WM_RBUTTONUP:
                case WM_CONTEXTMENU:
                {
                    ShowContextMenu();
                    break;
                }


            }
            break;
        }

        case WM_TOX ... WM_TOX + 128:
        {
            tox_message(msg - WM_TOX, wParam >> 16, wParam, (void*)lParam);
            break;
        }
    }

    return DefWindowProc(hwnd, msg, wParam, lParam);
}