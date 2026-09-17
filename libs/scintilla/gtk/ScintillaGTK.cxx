#ifndef NO_ACCESSIBILITY
#define NO_ACCESSIBILITY 1
#endif
// Scintilla source code edit control
// ScintillaGTK.cxx - GTK+ specific subclass of ScintillaBase
// Copyright 1998-2004 by Neil Hodgson <neilh@scintilla.org>
// The License.txt file describes the conditions under which this software may be distributed.

#include <cstddef>
#include <cstdlib>
#include <cassert>
#include <cstring>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <cmath>

#include <stdexcept>
#include <new>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>

#include <glib.h>
#include <gmodule.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#if defined(GDK_WINDOWING_WAYLAND)
#include <gdk/gdkwayland.h>
#endif

#if defined(__WIN32__) || defined(_MSC_VER)
#include <windows.h>
#endif

#include "Platform.h"

#include "ILoader.h"
#include "ILexer.h"
#include "Scintilla.h"
#include "ScintillaWidget.h"
#ifdef SCI_LEXER
#include "SciLexer.h"
#endif
#include "StringCopy.h"
#ifdef SCI_LEXER
#include "LexerModule.h"
#endif
#include "Position.h"
#include "UniqueString.h"
#include "SplitVector.h"
#include "Partitioning.h"
#include "RunStyles.h"
#include "ContractionState.h"
#include "CellBuffer.h"
#include "CallTip.h"
#include "KeyMap.h"
#include "Indicator.h"
#include "LineMarker.h"
#include "Style.h"
#include "ViewStyle.h"
#include "CharClassify.h"
#include "Decoration.h"
#include "CaseFolder.h"
#include "Document.h"
#include "CaseConvert.h"
#include "UniConversion.h"
#include "Selection.h"
#include "PositionCache.h"
#include "EditModel.h"
#include "MarginView.h"
#include "EditView.h"
#include "Editor.h"
#include "AutoComplete.h"
#include "ScintillaBase.h"

#ifdef SCI_LEXER
#include "ExternalLexer.h"
#endif

#include "ScintillaGTK.h"
#include "scintilla-marshal.h"

#include "Converter.h"

#define IS_WIDGET_REALIZED(w) (gtk_widget_get_realized(GTK_WIDGET(w)))
#define IS_WIDGET_MAPPED(w) (gtk_widget_get_mapped(GTK_WIDGET(w)))

#define SC_INDICATOR_INPUT INDIC_IME
#define SC_INDICATOR_TARGET INDIC_IME+1
#define SC_INDICATOR_CONVERTED INDIC_IME+2
#define SC_INDICATOR_UNKNOWN INDIC_IME_MAX

static GdkWindow *WindowFromWidget(GtkWidget *w) {
	return gtk_widget_get_window(w);
}

#ifdef _MSC_VER
// Constant conditional expressions are because of GTK+ headers
#pragma warning(disable: 4127)
// Ignore unreferenced local functions in GTK+ headers
#pragma warning(disable: 4505)
#endif

using namespace Scintilla;

static GdkWindow *PWindow(const Window &w) {
	GtkWidget *widget = static_cast<GtkWidget *>(w.GetID());
	return gtk_widget_get_window(widget);
}

extern std::string UTF8FromLatin1(const char *s, int len);

enum {
    COMMAND_SIGNAL,
    NOTIFY_SIGNAL,
    LAST_SIGNAL
};

static gint scintilla_signals[LAST_SIGNAL] = { 0 };

enum {
    TARGET_STRING,
    TARGET_TEXT,
    TARGET_COMPOUND_TEXT,
    TARGET_UTF8_STRING,
    TARGET_URI
};

GdkAtom ScintillaGTK::atomClipboard = 0;
GdkAtom ScintillaGTK::atomUTF8 = 0;
GdkAtom ScintillaGTK::atomString = 0;
GdkAtom ScintillaGTK::atomUriList = 0;
GdkAtom ScintillaGTK::atomDROPFILES_DND = 0;

static const GtkTargetEntry clipboardCopyTargets[] = {
	{ (gchar *) "UTF8_STRING", 0, TARGET_UTF8_STRING },
	{ (gchar *) "STRING", 0, TARGET_STRING },
};
static const gint nClipboardCopyTargets = ELEMENTS(clipboardCopyTargets);

static const GtkTargetEntry clipboardPasteTargets[] = {
	{ (gchar *) "text/uri-list", 0, TARGET_URI },
	{ (gchar *) "UTF8_STRING", 0, TARGET_UTF8_STRING },
	{ (gchar *) "STRING", 0, TARGET_STRING },
};
static const gint nClipboardPasteTargets = ELEMENTS(clipboardPasteTargets);

static const GdkDragAction actionCopyOrMove = static_cast<GdkDragAction>(GDK_ACTION_COPY | GDK_ACTION_MOVE);

static GtkWidget *PWidget(Window &w) {
	return static_cast<GtkWidget *>(w.GetID());
}

ScintillaGTK *ScintillaGTK::FromWidget(GtkWidget *widget) {
	ScintillaObject *scio = SCINTILLA(widget);
	return static_cast<ScintillaGTK *>(scio->pscin);
}

ScintillaGTK::ScintillaGTK(_ScintillaObject *sci_) :
		adjustmentv(0), adjustmenth(0),
		verticalScrollBarWidth(30), horizontalScrollBarHeight(30),
		evbtn(nullptr), capturedMouse(false), dragWasDropped(false),
		lastKey(0), rectangularSelectionModifier(SCMOD_CTRL), parentClass(0),
		im_context(NULL), lastNonCommonScript(PANGO_SCRIPT_INVALID_CODE),
		lastWheelMouseDirection(0),
		wheelMouseIntensity(0),
		smoothScrollY(0),
		smoothScrollX(0),
		rgnUpdate(0),
		repaintFullWindow(false),
		styleIdleID(0),
		accessibilityEnabled(SC_ACCESSIBILITY_DISABLED),
		accessible(0) {
	sci = sci_;
	wMain = GTK_WIDGET(sci);

	rectangularSelectionModifier = SCMOD_ALT;

#if PLAT_GTK_WIN32
	// There does not seem to be a real standard for indicating that the clipboard
	// contains a rectangular selection, so copy Developer Studio.
	cfColumnSelect = static_cast<CLIPFORMAT>(
		::RegisterClipboardFormat("MSDEVColumnSelect"));

	// Get intellimouse parameters when running on win32; otherwise use
	// reasonable default
#ifndef SPI_GETWHEELSCROLLLINES
#define SPI_GETWHEELSCROLLLINES   104
#endif
	::SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &linesPerScroll, 0);
#else
	linesPerScroll = 4;
#endif
	lastWheelMouseTime.tv_sec = 0;
	lastWheelMouseTime.tv_usec = 0;

	Init();
}

ScintillaGTK::~ScintillaGTK() {
	if (styleIdleID) {
		g_source_remove(styleIdleID);
		styleIdleID = 0;
	}
	if (evbtn) {
		gdk_event_free(evbtn);
		evbtn = nullptr;
	}
	wPreedit.Destroy();
}

static void UnRefCursor(GdkCursor *cursor) {
#if GTK_CHECK_VERSION(3,0,0)
	g_object_unref(cursor);
#else
	gdk_cursor_unref(cursor);
#endif
}

void ScintillaGTK::RealizeThis(GtkWidget *widget) {
	//Platform::DebugPrintf("ScintillaGTK::realize this\n");
	gtk_widget_set_realized(widget, TRUE);

	GdkWindowAttr attrs;
	attrs.window_type = GDK_WINDOW_CHILD;
	GtkAllocation allocation;
	gtk_widget_get_allocation(widget, &allocation);
	attrs.x = allocation.x;
	attrs.y = allocation.y;
	attrs.width = allocation.width;
	attrs.height = allocation.height;
	attrs.wclass = GDK_INPUT_OUTPUT;
	attrs.visual = gtk_widget_get_visual(widget);
#if !GTK_CHECK_VERSION(3,0,0)
	attrs.colormap = gtk_widget_get_colormap(widget);
#endif
	attrs.event_mask = gtk_widget_get_events(widget) | GDK_EXPOSURE_MASK;
	GdkDisplay *pdisplay = gtk_widget_get_display(widget);
	GdkCursor *cursor = gdk_cursor_new_for_display(pdisplay, GDK_XTERM);
	attrs.cursor = cursor;

#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_set_window(widget, gdk_window_new(gtk_widget_get_parent_window(widget), &attrs,
		GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_CURSOR));
	gdk_window_set_user_data(gtk_widget_get_window(widget), widget);
	gdk_window_show(gtk_widget_get_window(widget));
	UnRefCursor(cursor);
#else
	widget->window = gdk_window_new(gtk_widget_get_parent_window(widget), &attrs,
		GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP | GDK_WA_CURSOR);
	gdk_window_set_user_data(widget->window, widget);
	widget->style = gtk_style_attach(widget->style, widget->window);
	gdk_window_set_background(widget->window, &widget->style->bg[GTK_STATE_NORMAL]);
	gdk_window_show(widget->window);
	UnRefCursor(cursor);
#endif

	im_context = gtk_im_multicontext_new();
	g_signal_connect(G_OBJECT(im_context), "commit",
		G_CALLBACK(Commit), this);
	g_signal_connect(G_OBJECT(im_context), "preedit_changed",
		G_CALLBACK(PreeditChanged), this);
	gtk_im_context_set_client_window(im_context, WindowFromWidget(widget));

	GtkWidget *widtxt = PWidget(wText);
	g_signal_connect_after(G_OBJECT(widtxt), "style_set",
		G_CALLBACK(ScintillaGTK::StyleSetText), NULL);
	g_signal_connect_after(G_OBJECT(widtxt), "realize",
		G_CALLBACK(ScintillaGTK::RealizeText), NULL);

	gtk_widget_realize(widtxt);
	if (PWidget(scrollbarv)) gtk_widget_realize(PWidget(scrollbarv));
	if (PWidget(scrollbarh)) gtk_widget_realize(PWidget(scrollbarh));

	cursor = gdk_cursor_new_for_display(pdisplay, GDK_XTERM);
	gdk_window_set_cursor(PWindow(wText), cursor);
	UnRefCursor(cursor);

	if (PWidget(scrollbarv)) {
		cursor = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
		gdk_window_set_cursor(PWindow(scrollbarv), cursor);
		UnRefCursor(cursor);
	}

	if (PWidget(scrollbarh)) {
		cursor = gdk_cursor_new_for_display(pdisplay, GDK_LEFT_PTR);
		gdk_window_set_cursor(PWindow(scrollbarh), cursor);
		UnRefCursor(cursor);
	}

	wSelection = gtk_invisible_new();
	g_signal_connect(PWidget(wSelection), "selection_get", G_CALLBACK(PrimarySelection), (gpointer) this);
	g_signal_connect(PWidget(wSelection), "selection_clear_event", G_CALLBACK(PrimaryClear), (gpointer) this);
	gtk_selection_add_targets(PWidget(wSelection), GDK_SELECTION_PRIMARY,
	                          clipboardCopyTargets, nClipboardCopyTargets);
}

void ScintillaGTK::Realize(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->RealizeThis(widget);
}

void ScintillaGTK::UnRealizeThis(GtkWidget *widget) {
	try {
		gtk_selection_clear_targets(PWidget(wSelection), GDK_SELECTION_PRIMARY);
		wSelection.Destroy();

		if (IS_WIDGET_MAPPED(widget)) {
			gtk_widget_unmap(widget);
		}
		gtk_widget_set_realized(widget, FALSE);
		gtk_widget_unrealize(PWidget(wText));
		if (PWidget(scrollbarv))
			gtk_widget_unrealize(PWidget(scrollbarv));
		if (PWidget(scrollbarh))
			gtk_widget_unrealize(PWidget(scrollbarh));
		g_object_unref(im_context);
		im_context = NULL;
		if (GTK_WIDGET_CLASS(parentClass)->unrealize)
			GTK_WIDGET_CLASS(parentClass)->unrealize(widget);

		Finalise();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::UnRealize(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->UnRealizeThis(widget);
}

static void MapWidget(GtkWidget *widget) {
	if (widget &&
	        gtk_widget_get_visible(GTK_WIDGET(widget)) &&
	        !IS_WIDGET_MAPPED(widget)) {
		gtk_widget_map(widget);
	}
}

void ScintillaGTK::MapThis() {
	try {
		//Platform::DebugPrintf("ScintillaGTK::map this\n");
		gtk_widget_set_mapped(PWidget(wMain), TRUE);
		MapWidget(PWidget(wText));
		MapWidget(PWidget(scrollbarh));
		MapWidget(PWidget(scrollbarv));
		wMain.SetCursor(Window::cursorArrow);
		scrollbarv.SetCursor(Window::cursorArrow);
		scrollbarh.SetCursor(Window::cursorArrow);
		ChangeSize();
		gdk_window_show(PWindow(wMain));
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Map(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->MapThis();
}

void ScintillaGTK::UnMapThis() {
	try {
		//Platform::DebugPrintf("ScintillaGTK::unmap this\n");
		gtk_widget_set_mapped(PWidget(wMain), FALSE);
		DropGraphics(false);
		gdk_window_hide(PWindow(wMain));
		gtk_widget_unmap(PWidget(wText));
		if (PWidget(scrollbarh))
			gtk_widget_unmap(PWidget(scrollbarh));
		if (PWidget(scrollbarv))
			gtk_widget_unmap(PWidget(scrollbarv));
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::UnMap(GtkWidget *widget) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->UnMapThis();
}

void ScintillaGTK::ForAll(GtkCallback callback, gpointer callback_data) {
	try {
		(*callback) (PWidget(wText), callback_data);
		if (PWidget(scrollbarv))
			(*callback) (PWidget(scrollbarv), callback_data);
		if (PWidget(scrollbarh))
			(*callback) (PWidget(scrollbarh), callback_data);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::MainForAll(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data) {
	ScintillaGTK *sciThis = FromWidget((GtkWidget *)container);

	if (callback != NULL && include_internals) {
		sciThis->ForAll(callback, callback_data);
	}
}

namespace {

class PreEditString {
public:
	gchar *str;
	gint cursor_pos;
	PangoAttrList *attrs;
	gboolean validUTF8;
	glong uniStrLen;
	gunichar *uniStr;
	PangoScript pscript;

	explicit PreEditString(GtkIMContext *im_context) {
		gtk_im_context_get_preedit_string(im_context, &str, &attrs, &cursor_pos);
		validUTF8 = g_utf8_validate(str, strlen(str), NULL);
		uniStr = g_utf8_to_ucs4_fast(str, strlen(str), &uniStrLen);
		pscript = pango_script_for_unichar(uniStr[0]);
	}
	~PreEditString() {
		g_free(str);
		g_free(uniStr);
		pango_attr_list_unref(attrs);
	}
};

}

gint ScintillaGTK::FocusInThis(GtkWidget *) {
	try {
		SetFocusState(true);
		if (im_context != NULL) {
			PreEditString pes(im_context);
			if (PWidget(wPreedit) != NULL) {
				if (strlen(pes.str) > 0) {
					gtk_widget_show(PWidget(wPreedit));
				} else {
					gtk_widget_hide(PWidget(wPreedit));
				}
			}
			gtk_im_context_focus_in(im_context);
		}

	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gint ScintillaGTK::FocusIn(GtkWidget *widget, GdkEventFocus * /*event*/) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->FocusInThis(widget);
}

gint ScintillaGTK::FocusOutThis(GtkWidget *) {
	try {
		SetFocusState(false);

		if (PWidget(wPreedit) != NULL)
			gtk_widget_hide(PWidget(wPreedit));
		if (im_context != NULL)
			gtk_im_context_focus_out(im_context);

	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gint ScintillaGTK::FocusOut(GtkWidget *widget, GdkEventFocus * /*event*/) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->FocusOutThis(widget);
}

void ScintillaGTK::SizeRequest(GtkWidget *widget, GtkRequisition *requisition) {
	ScintillaGTK *sciThis = FromWidget(widget);
	requisition->width = 1;
	requisition->height = 1;
	GtkRequisition child_requisition;
#if GTK_CHECK_VERSION(3,0,0)
	gtk_widget_get_preferred_size(PWidget(sciThis->scrollbarh), NULL, &child_requisition);
	gtk_widget_get_preferred_size(PWidget(sciThis->scrollbarv), NULL, &child_requisition);
#else
	gtk_widget_size_request(PWidget(sciThis->scrollbarh), &child_requisition);
	gtk_widget_size_request(PWidget(sciThis->scrollbarv), &child_requisition);
#endif
}

#if GTK_CHECK_VERSION(3,0,0)

void ScintillaGTK::GetPreferredWidth(GtkWidget *widget, gint *minimalWidth, gint *naturalWidth) {
	GtkRequisition requisition;
	SizeRequest(widget, &requisition);
	*minimalWidth = *naturalWidth = requisition.width;
}

void ScintillaGTK::GetPreferredHeight(GtkWidget *widget, gint *minimalHeight, gint *naturalHeight) {
	GtkRequisition requisition;
	SizeRequest(widget, &requisition);
	*minimalHeight = *naturalHeight = requisition.height;
}

#endif

void ScintillaGTK::SizeAllocate(GtkWidget *widget, GtkAllocation *allocation) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		gtk_widget_set_allocation(widget, allocation);
		if (IS_WIDGET_REALIZED(widget))
			gdk_window_move_resize(WindowFromWidget(widget),
			        allocation->x,
			        allocation->y,
			        allocation->width,
			        allocation->height);

		sciThis->Resize(allocation->width, allocation->height);

	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Init() {
	parentClass = reinterpret_cast<GtkWidgetClass *>(
	                  g_type_class_ref(gtk_container_get_type()));

	gint maskSmooth = 0;
#if defined(GDK_WINDOWING_WAYLAND)
	GdkDisplay *pdisplay = gdk_display_get_default();
	if (GDK_IS_WAYLAND_DISPLAY(pdisplay)) {
		// On Wayland, touch pads only produce smooth scroll events
		maskSmooth = GDK_SMOOTH_SCROLL_MASK;
	}
#endif

	gtk_widget_set_can_focus(PWidget(wMain), TRUE);
	gtk_widget_set_sensitive(PWidget(wMain), TRUE);
	gtk_widget_set_events(PWidget(wMain),
	                      GDK_EXPOSURE_MASK
	                      | GDK_SCROLL_MASK
	                      | maskSmooth
	                      | GDK_STRUCTURE_MASK
	                      | GDK_KEY_PRESS_MASK
	                      | GDK_KEY_RELEASE_MASK
	                      | GDK_FOCUS_CHANGE_MASK
	                      | GDK_LEAVE_NOTIFY_MASK
	                      | GDK_BUTTON_PRESS_MASK
	                      | GDK_BUTTON_RELEASE_MASK
	                      | GDK_POINTER_MOTION_MASK
	                      | GDK_POINTER_MOTION_HINT_MASK);

	wText = gtk_drawing_area_new();
	gtk_widget_set_parent(PWidget(wText), PWidget(wMain));
	GtkWidget *widtxt = PWidget(wText);	// No code inside the G_OBJECT macro
	gtk_widget_show(widtxt);
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(G_OBJECT(widtxt), "draw",
			   G_CALLBACK(ScintillaGTK::DrawText), this);
#else
	g_signal_connect(G_OBJECT(widtxt), "expose_event",
			   G_CALLBACK(ScintillaGTK::ExposeText), this);
#endif
#if GTK_CHECK_VERSION(3,0,0)
	// we need a runtime check because we don't want double buffering when
	// running on >= 3.9.2
	if (gtk_check_version(3,9,2) != NULL /* on < 3.9.2 */)
#endif
	{
#if !GTK_CHECK_VERSION(3,14,0)
		// Avoid background drawing flash/missing redraws
		gtk_widget_set_double_buffered(widtxt, FALSE);
#endif
	}
	gtk_widget_set_events(widtxt, GDK_EXPOSURE_MASK);
	gtk_widget_set_size_request(widtxt, 100, 100);
	adjustmentv = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 201.0, 1.0, 20.0, 20.0));
#if GTK_CHECK_VERSION(3,0,0)
	scrollbarv = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, GTK_ADJUSTMENT(adjustmentv));
#else
	scrollbarv = gtk_vscrollbar_new(GTK_ADJUSTMENT(adjustmentv));
#endif
	gtk_widget_set_can_focus(PWidget(scrollbarv), FALSE);
	g_signal_connect(G_OBJECT(adjustmentv), "value_changed",
			   G_CALLBACK(ScrollSignal), this);
	gtk_widget_set_parent(PWidget(scrollbarv), PWidget(wMain));
	gtk_widget_show(PWidget(scrollbarv));

	adjustmenth = GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 101.0, 1.0, 20.0, 20.0));
#if GTK_CHECK_VERSION(3,0,0)
	scrollbarh = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_ADJUSTMENT(adjustmenth));
#else
	scrollbarh = gtk_hscrollbar_new(GTK_ADJUSTMENT(adjustmenth));
#endif
	gtk_widget_set_can_focus(PWidget(scrollbarh), FALSE);
	g_signal_connect(G_OBJECT(adjustmenth), "value_changed",
			   G_CALLBACK(ScrollHSignal), this);
	gtk_widget_set_parent(PWidget(scrollbarh), PWidget(wMain));
	gtk_widget_show(PWidget(scrollbarh));

	gtk_widget_grab_focus(PWidget(wMain));

	gtk_drag_dest_set(GTK_WIDGET(PWidget(wMain)),
	                  GTK_DEST_DEFAULT_ALL, clipboardPasteTargets, nClipboardPasteTargets,
	                  actionCopyOrMove);

	/* create pre-edit window */
	wPreedit = gtk_window_new(GTK_WINDOW_POPUP);
	wPreeditDraw = gtk_drawing_area_new();
	GtkWidget *predrw = PWidget(wPreeditDraw);      // No code inside the G_OBJECT macro
#if GTK_CHECK_VERSION(3,0,0)
	g_signal_connect(G_OBJECT(predrw), "draw",
		G_CALLBACK(DrawPreedit), this);
#else
	g_signal_connect(G_OBJECT(predrw), "expose_event",
		G_CALLBACK(ExposePreedit), this);
#endif
	gtk_container_add(GTK_CONTAINER(PWidget(wPreedit)), predrw);
	gtk_widget_show(predrw);

	// Set caret period based on GTK settings
	gboolean blinkOn = false;
	if (g_object_class_find_property(G_OBJECT_GET_CLASS(
			G_OBJECT(gtk_settings_get_default())), "gtk-cursor-blink")) {
		g_object_get(G_OBJECT(
			gtk_settings_get_default()), "gtk-cursor-blink", &blinkOn, NULL);
	}
	if (blinkOn &&
		g_object_class_find_property(G_OBJECT_GET_CLASS(
			G_OBJECT(gtk_settings_get_default())), "gtk-cursor-blink-time")) {
		gint value;
		g_object_get(G_OBJECT(
			gtk_settings_get_default()), "gtk-cursor-blink-time", &value, NULL);
		caret.period = gint(value / 1.75);
	} else {
		caret.period = 0;
	}

	for (TickReason tr = tickCaret; tr <= tickDwell; tr = static_cast<TickReason>(tr + 1)) {
		timers[tr].reason = tr;
		timers[tr].scintilla = this;
	}
	vs.indicators[SC_INDICATOR_UNKNOWN] = Indicator(INDIC_HIDDEN, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_INPUT] = Indicator(INDIC_DOTS, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_CONVERTED] = Indicator(INDIC_COMPOSITIONTHICK, ColourDesired(0, 0, 0xff));
	vs.indicators[SC_INDICATOR_TARGET] = Indicator(INDIC_STRAIGHTBOX, ColourDesired(0, 0, 0xff));
}

void ScintillaGTK::Finalise() {
	for (TickReason tr = tickCaret; tr <= tickDwell; tr = static_cast<TickReason>(tr + 1)) {
		FineTickerCancel(tr);
	}
	accessible = 0;
	ScintillaBase::Finalise();
}

bool ScintillaGTK::AbandonPaint() {
	if ((paintState == painting) && !paintingAllText) {
		repaintFullWindow = true;
	}
	return false;
}

void ScintillaGTK::DisplayCursor(Window::Cursor c) {
	if (cursorMode == SC_CURSORNORMAL)
		wText.SetCursor(c);
	else
		wText.SetCursor(static_cast<Window::Cursor>(cursorMode));
}

bool ScintillaGTK::DragThreshold(Point ptStart, Point ptNow) {
	return gtk_drag_check_threshold(GTK_WIDGET(PWidget(wMain)),
		ptStart.x, ptStart.y, ptNow.x, ptNow.y);
}

void ScintillaGTK::StartDrag() {
	PLATFORM_ASSERT(evbtn);
	dragWasDropped = false;
	inDragDrop = ddDragging;
	GtkTargetList *tl = gtk_target_list_new(clipboardCopyTargets, nClipboardCopyTargets);
#if GTK_CHECK_VERSION(3,10,0)
	gtk_drag_begin_with_coordinates(GTK_WIDGET(PWidget(wMain)),
	               tl,
	               actionCopyOrMove,
	               buttonMouse,
	               evbtn,
			-1, -1);
#else
	gtk_drag_begin(GTK_WIDGET(PWidget(wMain)),
	               tl,
	               actionCopyOrMove,
	               buttonMouse,
	               evbtn);
#endif
}

namespace Scintilla {
std::string ConvertText(const char *s, size_t len, const char *charSetDest,
	const char *charSetSource, bool transliterations, bool silent) {
	std::string destForm;
	Converter conv(charSetDest, charSetSource, transliterations);
	if (conv) {
		gsize outLeft = len*3+1;
		destForm = std::string(outLeft, '\0');
		char *pin = const_cast<char *>(s);
		gsize inLeft = len;
		char *putf = &destForm[0];
		char *pout = putf;
		gsize conversions = conv.Convert(&pin, &inLeft, &pout, &outLeft);
		if (conversions == sizeFailure) {
			if (!silent) {
				if (len == 1)
					fprintf(stderr, "iconv %s->%s failed for %0x '%s'\n",
						charSetSource, charSetDest, (unsigned char)(*s), s);
				else
					fprintf(stderr, "iconv %s->%s failed for %s\n",
						charSetSource, charSetDest, s);
			}
			destForm = std::string();
		} else {
			destForm.resize(pout - putf);
		}
	} else {
		fprintf(stderr, "Can not iconv %s %s\n", charSetDest, charSetSource);
	}
	return destForm;
}
}

Sci::Position ScintillaGTK::TargetAsUTF8(char *text) const {
	Sci::Position targetLength = targetEnd - targetStart;
	if (IsUnicodeMode()) {
		if (text) {
			pdoc->GetCharRange(text, targetStart, targetLength);
		}
	} else {
		const char *charSetBuffer = CharacterSetID();
		if (*charSetBuffer) {
			std::string s = RangeText(targetStart, targetEnd);
			std::string tmputf = ConvertText(&s[0], targetLength, "UTF-8", charSetBuffer, false);
			if (text) {
				memcpy(text, tmputf.c_str(), tmputf.length());
			}
			return tmputf.length();
		} else {
			if (text) {
				pdoc->GetCharRange(text, targetStart, targetLength);
			}
		}
	}
	return targetLength;
}

Sci::Position ScintillaGTK::EncodedFromUTF8(const char *utf8, char *encoded) const {
	Sci::Position inputLength = (lengthForEncode >= 0) ? lengthForEncode : strlen(utf8);
	if (IsUnicodeMode()) {
		if (encoded) {
			memcpy(encoded, utf8, inputLength);
		}
		return inputLength;
	} else {
		const char *charSetBuffer = CharacterSetID();
		if (*charSetBuffer) {
			std::string tmpEncoded = ConvertText(utf8, inputLength, charSetBuffer, "UTF-8", true);
			if (encoded) {
				memcpy(encoded, tmpEncoded.c_str(), tmpEncoded.length());
			}
			return tmpEncoded.length();
		} else {
			if (encoded) {
				memcpy(encoded, utf8, inputLength);
			}
			return inputLength;
		}
	}
	return 0;
}

bool ScintillaGTK::ValidCodePage(int codePage) const {
	return codePage == 0
	|| codePage == SC_CP_UTF8
	|| codePage == 932
	|| codePage == 936
	|| codePage == 949
	|| codePage == 950
	|| codePage == 1361;
}

sptr_t ScintillaGTK::WndProc(unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	try {
		switch (iMessage) {

		case SCI_GRABFOCUS:
			gtk_widget_grab_focus(PWidget(wMain));
			break;

		case SCI_GETDIRECTFUNCTION:
			return reinterpret_cast<sptr_t>(DirectFunction);

		case SCI_GETDIRECTPOINTER:
			return reinterpret_cast<sptr_t>(this);

#ifdef SCI_LEXER
		case SCI_LOADLEXERLIBRARY:
			LexerManager::GetInstance()->Load(ConstCharPtrFromSPtr(lParam));
			break;
#endif
		case SCI_TARGETASUTF8:
			return TargetAsUTF8(CharPtrFromSPtr(lParam));

		case SCI_ENCODEDFROMUTF8:
			return EncodedFromUTF8(ConstCharPtrFromUPtr(wParam),
			        CharPtrFromSPtr(lParam));

		case SCI_SETRECTANGULARSELECTIONMODIFIER:
			rectangularSelectionModifier = wParam;
			break;

		case SCI_GETRECTANGULARSELECTIONMODIFIER:
			return rectangularSelectionModifier;

		case SCI_SETREADONLY: {
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
		}

		case SCI_GETACCESSIBILITY:
			return SC_ACCESSIBILITY_DISABLED;

		case SCI_SETACCESSIBILITY:
			accessibilityEnabled = SC_ACCESSIBILITY_DISABLED;
			break;

		default:
			return ScintillaBase::WndProc(iMessage, wParam, lParam);
		}
	} catch (std::bad_alloc&) {
		errorStatus = SC_STATUS_BADALLOC;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return 0;
}

sptr_t ScintillaGTK::DefWndProc(unsigned int, uptr_t, sptr_t) {
	return 0;
}

bool ScintillaGTK::FineTickerRunning(TickReason reason) {
	return timers[reason].timer != 0;
}

void ScintillaGTK::FineTickerStart(TickReason reason, int millis, int /* tolerance */) {
	FineTickerCancel(reason);
	timers[reason].timer = gdk_threads_add_timeout(millis, TimeOut, &timers[reason]);
}

void ScintillaGTK::FineTickerCancel(TickReason reason) {
	if (timers[reason].timer) {
		g_source_remove(timers[reason].timer);
		timers[reason].timer = 0;
	}
}

bool ScintillaGTK::SetIdle(bool on) {
	if (on) {
		if (!idler.state) {
			idler.state = true;
			idler.idlerID = GUINT_TO_POINTER(
				gdk_threads_add_idle_full(G_PRIORITY_DEFAULT_IDLE, IdleCallback, this, NULL));
		}
	} else {
		if (idler.state) {
			idler.state = false;
			g_source_remove(GPOINTER_TO_UINT(idler.idlerID));
		}
	}
	return true;
}

void ScintillaGTK::SetMouseCapture(bool on) {
	if (mouseDownCaptures) {
		if (on) {
			gtk_grab_add(GTK_WIDGET(PWidget(wMain)));
		} else {
			gtk_grab_remove(GTK_WIDGET(PWidget(wMain)));
		}
	}
	capturedMouse = on;
}

bool ScintillaGTK::HaveMouseCapture() {
	return capturedMouse;
}

#if GTK_CHECK_VERSION(3,0,0)

static bool CRectContains(const cairo_rectangle_t &crcContainer, const cairo_rectangle_t &crcTest) {
	return
		(crcTest.x >= crcContainer.x) && ((crcTest.x + crcTest.width) <= (crcContainer.x + crcContainer.width)) &&
		(crcTest.y >= crcContainer.y) && ((crcTest.y + crcTest.height) <= (crcContainer.y + crcContainer.height));
}

static bool CRectListContains(const cairo_rectangle_list_t *crcListContainer, const cairo_rectangle_t &crcTest) {
	for (int r=0; r<crcListContainer->num_rectangles; r++) {
		if (CRectContains(crcListContainer->rectangles[r], crcTest))
			return true;
	}
	return false;
}

#endif

bool ScintillaGTK::PaintContains(PRectangle rc) {
	bool contains = true;
	if (paintState == painting) {
		if (!rcPaint.Contains(rc)) {
			contains = false;
		} else if (rgnUpdate) {
#if GTK_CHECK_VERSION(3,0,0)
			cairo_rectangle_t grc = {rc.left, rc.top,
				rc.right - rc.left, rc.bottom - rc.top};
			contains = CRectListContains(rgnUpdate, grc);
#else
			GdkRectangle grc = {static_cast<gint>(rc.left), static_cast<gint>(rc.top),
				static_cast<gint>(rc.right - rc.left), static_cast<gint>(rc.bottom - rc.top)};
			if (gdk_region_rect_in(rgnUpdate, &grc) != GDK_OVERLAP_RECTANGLE_IN) {
				contains = false;
			}
#endif
		}
	}
	return contains;
}

void ScintillaGTK::FullPaint() {
	wText.InvalidateAll();
}

PRectangle ScintillaGTK::GetClientRectangle() const {
	PRectangle rc = wMain.GetClientPosition();
	if (verticalScrollBarVisible)
		rc.right -= verticalScrollBarWidth;
	if (horizontalScrollBarVisible && !Wrapping())
		rc.bottom -= horizontalScrollBarHeight;
	rc.right -= rc.left;
	rc.bottom -= rc.top;
	if (rc.bottom < 0)
		rc.bottom = 0;
	if (rc.right < 0)
		rc.right = 0;
	rc.left = 0;
	rc.top = 0;
	return rc;
}

void ScintillaGTK::ScrollText(Sci::Line linesToMove) {
	NotifyUpdateUI();

#if GTK_CHECK_VERSION(3,22,0)
	Redraw();
#else
	GtkWidget *wi = PWidget(wText);
	if (IS_WIDGET_REALIZED(wi)) {
		const int diff = vs.lineHeight * -linesToMove;
		gdk_window_scroll(WindowFromWidget(wi), 0, -diff);
		gdk_window_process_updates(WindowFromWidget(wi), FALSE);
	}
#endif
}

void ScintillaGTK::SetVerticalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmentv), topLine);
}

void ScintillaGTK::SetHorizontalScrollPos() {
	DwellEnd(true);
	gtk_adjustment_set_value(GTK_ADJUSTMENT(adjustmenth), xOffset);
}

bool ScintillaGTK::ModifyScrollBars(Sci::Line nMax, Sci::Line nPage) {
	bool modified = false;
	int pageScroll = LinesToScroll();

	if (gtk_adjustment_get_upper(adjustmentv) != (nMax + 1) ||
	        gtk_adjustment_get_page_size(adjustmentv) != nPage ||
	        gtk_adjustment_get_page_increment(adjustmentv) != pageScroll) {
		gtk_adjustment_set_upper(adjustmentv, nMax + 1);
	        gtk_adjustment_set_page_size(adjustmentv, nPage);
	        gtk_adjustment_set_page_increment(adjustmentv, pageScroll);
#if !GTK_CHECK_VERSION(3,18,0)
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmentv));
#endif
		modified = true;
	}

	PRectangle rcText = GetTextRectangle();
	int horizEndPreferred = scrollWidth;
	if (horizEndPreferred < 0)
		horizEndPreferred = 0;
	unsigned int pageWidth = rcText.Width();
	unsigned int pageIncrement = pageWidth / 3;
	unsigned int charWidth = vs.styles[STYLE_DEFAULT].aveCharWidth;
	if (gtk_adjustment_get_upper(adjustmenth) != horizEndPreferred ||
	        gtk_adjustment_get_page_size(adjustmenth) != pageWidth ||
	        gtk_adjustment_get_page_increment(adjustmenth) != pageIncrement ||
	        gtk_adjustment_get_step_increment(adjustmenth) != charWidth) {
		gtk_adjustment_set_upper(adjustmenth, horizEndPreferred);
	        gtk_adjustment_set_page_size(adjustmenth, pageWidth);
	        gtk_adjustment_set_page_increment(adjustmenth, pageIncrement);
	        gtk_adjustment_set_step_increment(adjustmenth, charWidth);
#if !GTK_CHECK_VERSION(3,18,0)
		gtk_adjustment_changed(GTK_ADJUSTMENT(adjustmenth));
#endif
		modified = true;
	}
	if (modified && (paintState == painting)) {
		repaintFullWindow = true;
	}

	return modified;
}

void ScintillaGTK::ReconfigureScrollBars() {
	PRectangle rc = wMain.GetClientPosition();
	Resize(rc.Width(), rc.Height());
}

void ScintillaGTK::NotifyChange() {
	g_signal_emit(G_OBJECT(sci), scintilla_signals[COMMAND_SIGNAL], 0,
	                Platform::LongFromTwoShorts(GetCtrlID(), SCEN_CHANGE), PWidget(wMain));
}

void ScintillaGTK::NotifyFocus(bool focus) {
	g_signal_emit(G_OBJECT(sci), scintilla_signals[COMMAND_SIGNAL], 0,
	                Platform::LongFromTwoShorts
					(GetCtrlID(), focus ? SCEN_SETFOCUS : SCEN_KILLFOCUS), PWidget(wMain));
	Editor::NotifyFocus(focus);
}

void ScintillaGTK::NotifyParent(SCNotification scn) {
	scn.nmhdr.hwndFrom = PWidget(wMain);
	scn.nmhdr.idFrom = GetCtrlID();
	g_signal_emit(G_OBJECT(sci), scintilla_signals[NOTIFY_SIGNAL], 0,
	                GetCtrlID(), &scn);
}

void ScintillaGTK::NotifyKey(int key, int modifiers) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_KEY;
	scn.ch = key;
	scn.modifiers = modifiers;

	NotifyParent(scn);
}

void ScintillaGTK::NotifyURIDropped(const char *list) {
	SCNotification scn = {};
	scn.nmhdr.code = SCN_URIDROPPED;
	scn.text = list;

	NotifyParent(scn);
}

const char *CharacterSetID(int characterSet);

const char *ScintillaGTK::CharacterSetID() const {
	return ::CharacterSetID(vs.styles[STYLE_DEFAULT].characterSet);
}

class CaseFolderDBCS : public CaseFolderTable {
	const char *charSet;
public:
	explicit CaseFolderDBCS(const char *charSet_) : charSet(charSet_) {
		StandardASCII();
	}
	size_t Fold(char *folded, size_t sizeFolded, const char *mixed, size_t lenMixed) override {
		if ((lenMixed == 1) && (sizeFolded > 0)) {
			folded[0] = mapping[static_cast<unsigned char>(mixed[0])];
			return 1;
		} else if (*charSet) {
			std::string sUTF8 = ConvertText(mixed, lenMixed,
				"UTF-8", charSet, false);
			if (!sUTF8.empty()) {
				gchar *mapped = g_utf8_casefold(sUTF8.c_str(), sUTF8.length());
				size_t lenMapped = strlen(mapped);
				if (lenMapped < sizeFolded) {
					memcpy(folded, mapped,  lenMapped);
				} else {
					folded[0] = '\0';
					lenMapped = 1;
				}
				g_free(mapped);
				return lenMapped;
			}
		}
		folded[0] = '\0';
		return 1;
	}
};

CaseFolder *ScintillaGTK::CaseFolderForEncoding() {
	if (pdoc->dbcsCodePage == SC_CP_UTF8) {
		return new CaseFolderUnicode();
	} else {
		const char *charSetBuffer = CharacterSetID();
		if (charSetBuffer) {
			if (pdoc->dbcsCodePage == 0) {
				CaseFolderTable *pcf = new CaseFolderTable();
				pcf->StandardASCII();
				for (int i=0x80; i<0x100; i++) {
					char sCharacter[2] = "A";
					sCharacter[0] = i;
					std::string sUTF8 = ConvertText(sCharacter, 1,
						"UTF-8", charSetBuffer, false, true);
					if (!sUTF8.empty()) {
						gchar *mapped = g_utf8_casefold(sUTF8.c_str(), sUTF8.length());
						if (mapped) {
							std::string mappedBack = ConvertText(mapped, strlen(mapped),
								charSetBuffer, "UTF-8", false, true);
							if ((mappedBack.length() == 1) && (mappedBack[0] != sCharacter[0])) {
								pcf->SetTranslation(sCharacter[0], mappedBack[0]);
							}
							g_free(mapped);
						}
					}
				}
				return pcf;
			} else {
				return new CaseFolderDBCS(charSetBuffer);
			}
		}
		return 0;
	}
}

namespace {

struct CaseMapper {
	gchar *mapped;
	CaseMapper(const std::string &sUTF8, bool toUpperCase) {
		if (toUpperCase) {
			mapped = g_utf8_strup(sUTF8.c_str(), sUTF8.length());
		} else {
			mapped = g_utf8_strdown(sUTF8.c_str(), sUTF8.length());
		}
	}
	~CaseMapper() {
		g_free(mapped);
	}
};

}

std::string ScintillaGTK::CaseMapString(const std::string &s, int caseMapping) {
	if ((s.size() == 0) || (caseMapping == cmSame))
		return s;

	if (IsUnicodeMode()) {
		std::string retMapped(s.length() * maxExpansionCaseConversion, 0);
		size_t lenMapped = CaseConvertString(&retMapped[0], retMapped.length(), s.c_str(), s.length(),
			(caseMapping == cmUpper) ? CaseConversionUpper : CaseConversionLower);
		retMapped.resize(lenMapped);
		return retMapped;
	}

	const char *charSetBuffer = CharacterSetID();

	if (!*charSetBuffer) {
		CaseMapper mapper(s, caseMapping == cmUpper);
		return std::string(mapper.mapped, strlen(mapper.mapped));
	} else {
		std::string sUTF8 = ConvertText(s.c_str(), s.length(),
			"UTF-8", charSetBuffer, false);
		CaseMapper mapper(sUTF8, caseMapping == cmUpper);
		return ConvertText(mapper.mapped, strlen(mapper.mapped), charSetBuffer, "UTF-8", false);
	}
}

int ScintillaGTK::KeyDefault(int key, int modifiers) {
	NotifyKey(key, modifiers);
	return 0;
}

void ScintillaGTK::CopyToClipboard(const SelectionText &selectedText) {
	SelectionText *clipText = new SelectionText();
	clipText->Copy(selectedText);
	StoreOnClipboard(clipText);
}

void ScintillaGTK::Copy() {
	if (!sel.Empty()) {
		SelectionText *clipText = new SelectionText();
		CopySelectionRange(clipText);
		StoreOnClipboard(clipText);
#if PLAT_GTK_WIN32
		if (sel.IsRectangular()) {
			::OpenClipboard(NULL);
			::SetClipboardData(cfColumnSelect, 0);
			::CloseClipboard();
		}
#endif
	}
}

void ScintillaGTK::Paste() {
	atomSought = atomUTF8;
	GtkClipboard *clipBoard =
		gtk_widget_get_clipboard(GTK_WIDGET(PWidget(wMain)), atomClipboard);
	if (clipBoard == NULL)
		return;

	class Helper : GObjectWatcher {
		ScintillaGTK *sci;

		void Destroyed() override {
			sci = 0;
		}

	public:
		Helper(ScintillaGTK *sci_) :
				GObjectWatcher(G_OBJECT(PWidget(sci_->wMain))),
				sci(sci_) {
		}

		static void ClipboardReceived(GtkClipboard *, GtkSelectionData *selection_data, gpointer data) {
			Helper *self = static_cast<Helper*>(data);
			if (self->sci != 0) {
				self->sci->ReceivedSelection(selection_data);
			}
			delete self;
		}
	};

	gtk_clipboard_request_contents(clipBoard, atomSought, Helper::ClipboardReceived, new Helper(this));
}

void ScintillaGTK::CreateCallTipWindow(PRectangle rc) {
	if (!ct.wCallTip.Created()) {
		ct.wCallTip = gtk_window_new(GTK_WINDOW_POPUP);
		ct.wDraw = gtk_drawing_area_new();
		GtkWidget *widcdrw = PWidget(ct.wDraw);
		gtk_container_add(GTK_CONTAINER(PWidget(ct.wCallTip)), widcdrw);
#if GTK_CHECK_VERSION(3,0,0)
		g_signal_connect(G_OBJECT(widcdrw), "draw",
				   G_CALLBACK(ScintillaGTK::DrawCT), &ct);
#else
		g_signal_connect(G_OBJECT(widcdrw), "expose_event",
				   G_CALLBACK(ScintillaGTK::ExposeCT), &ct);
#endif
		g_signal_connect(G_OBJECT(widcdrw), "button_press_event",
				   G_CALLBACK(ScintillaGTK::PressCT), this);
		gtk_widget_set_events(widcdrw,
			GDK_EXPOSURE_MASK | GDK_BUTTON_PRESS_MASK);
		GtkWidget *top = gtk_widget_get_toplevel(static_cast<GtkWidget *>(wMain.GetID()));
		gtk_window_set_transient_for(GTK_WINDOW(static_cast<GtkWidget *>(PWidget(ct.wCallTip))),
			GTK_WINDOW(top));
	}
	gtk_widget_set_size_request(PWidget(ct.wDraw), rc.Width(), rc.Height());
	ct.wDraw.Show();
	if (PWindow(ct.wCallTip)) {
		gdk_window_resize(PWindow(ct.wCallTip), rc.Width(), rc.Height());
	}
}

void ScintillaGTK::AddToPopUp(const char *label, int cmd, bool enabled) {
	GtkWidget *menuItem;
	if (label[0])
		menuItem = gtk_menu_item_new_with_label(label);
	else
		menuItem = gtk_separator_menu_item_new();
	gtk_menu_shell_append(GTK_MENU_SHELL(popup.GetID()), menuItem);
	g_object_set_data(G_OBJECT(menuItem), "CmdNum", GINT_TO_POINTER(cmd));
	g_signal_connect(G_OBJECT(menuItem),"activate", G_CALLBACK(PopUpCB), this);

	if (cmd) {
		if (menuItem)
			gtk_widget_set_sensitive(menuItem, enabled);
	}
}

bool ScintillaGTK::OwnPrimarySelection() {
	return (wSelection.Created() &&
		(gdk_selection_owner_get(GDK_SELECTION_PRIMARY) == PWindow(wSelection)) &&
		(PWindow(wSelection) != NULL));
}

void ScintillaGTK::ClaimSelection() {
	if (!sel.Empty() && wSelection.Created() && IS_WIDGET_REALIZED(GTK_WIDGET(PWidget(wSelection)))) {
		primarySelection = true;
		gtk_selection_owner_set(GTK_WIDGET(PWidget(wSelection)),
		                        GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
		primary.Clear();
	} else if (OwnPrimarySelection()) {
		primarySelection = true;
		if (primary.Empty())
			gtk_selection_owner_set(NULL, GDK_SELECTION_PRIMARY, GDK_CURRENT_TIME);
	} else {
		primarySelection = false;
		primary.Clear();
	}
}

static const guchar *DataOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_data(sd); }
static gint LengthOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_length(sd); }
static GdkAtom TypeOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_data_type(sd); }
static GdkAtom SelectionOfGSD(GtkSelectionData *sd) { return gtk_selection_data_get_selection(sd); }

void ScintillaGTK::GetGtkSelectionText(GtkSelectionData *selectionData, SelectionText &selText) {
	const char *data = reinterpret_cast<const char *>(DataOfGSD(selectionData));
	int len = LengthOfGSD(selectionData);
	GdkAtom selectionTypeData = TypeOfGSD(selectionData);

	if ((selectionTypeData != GDK_TARGET_STRING) && (selectionTypeData != atomUTF8)) {
		selText.Clear();
		return;
	}

	bool isRectangular;
#if PLAT_GTK_WIN32
	isRectangular = ::IsClipboardFormatAvailable(cfColumnSelect) != 0;
#else
	isRectangular = ((len > 2) && (data[len - 1] == 0 && data[len - 2] == '\n'));
	if (isRectangular)
		len--;
#endif

#if PLAT_GTK_WIN32
	if ((len > 0) && (data[len - 1] == '\0'))
		len--;
#endif

	std::string dest(data, len);
	if (selectionTypeData == GDK_TARGET_STRING) {
		if (IsUnicodeMode()) {
			dest = UTF8FromLatin1(dest.c_str(), dest.length());
			selText.Copy(dest, SC_CP_UTF8, 0, isRectangular, false);
		} else {
			selText.Copy(dest, pdoc->dbcsCodePage,
				vs.styles[STYLE_DEFAULT].characterSet, isRectangular, false);
		}
	} else {
		const char *charSetBuffer = CharacterSetID();
		if (!IsUnicodeMode() && *charSetBuffer) {
			dest = ConvertText(dest.c_str(), dest.length(), charSetBuffer, "UTF-8", true);
			selText.Copy(dest, pdoc->dbcsCodePage,
				vs.styles[STYLE_DEFAULT].characterSet, isRectangular, false);
		} else {
			selText.Copy(dest, SC_CP_UTF8, 0, isRectangular, false);
		}
	}
}

void ScintillaGTK::ReceivedSelection(GtkSelectionData *selection_data) {
	try {
		if ((SelectionOfGSD(selection_data) == atomClipboard) ||
		        (SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY)) {
			if ((atomSought == atomUTF8) && (LengthOfGSD(selection_data) <= 0)) {
				atomSought = atomString;
				gtk_selection_convert(GTK_WIDGET(PWidget(wMain)),
				        SelectionOfGSD(selection_data), atomSought, GDK_CURRENT_TIME);
			} else if ((LengthOfGSD(selection_data) > 0) &&
			        ((TypeOfGSD(selection_data) == GDK_TARGET_STRING) || (TypeOfGSD(selection_data) == atomUTF8))) {
				SelectionText selText;
				GetGtkSelectionText(selection_data, selText);

				UndoGroup ug(pdoc);
				if (SelectionOfGSD(selection_data) != GDK_SELECTION_PRIMARY) {
					ClearSelection(multiPasteMode == SC_MULTIPASTE_EACH);
				}

				InsertPasteShape(selText.Data(), selText.Length(),
					selText.rectangular ? pasteRectangular : pasteStream);
				EnsureCaretVisible();
			}
		}
		Redraw();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::ReceivedDrop(GtkSelectionData *selection_data) {
	dragWasDropped = true;
	if (TypeOfGSD(selection_data) == atomUriList || TypeOfGSD(selection_data) == atomDROPFILES_DND) {
		const char *data = reinterpret_cast<const char *>(DataOfGSD(selection_data));
		std::vector<char> drop(data, data + LengthOfGSD(selection_data));
		drop.push_back('\0');
		NotifyURIDropped(&drop[0]);
	} else if ((TypeOfGSD(selection_data) == GDK_TARGET_STRING) || (TypeOfGSD(selection_data) == atomUTF8)) {
		if (LengthOfGSD(selection_data) > 0) {
			SelectionText selText;
			GetGtkSelectionText(selection_data, selText);
			DropAt(posDrop, selText.Data(), selText.Length(), false, selText.rectangular);
		}
	}
	Redraw();
}

void ScintillaGTK::GetSelection(GtkSelectionData *selection_data, guint info, SelectionText *text) {
#if PLAT_GTK_WIN32
	std::unique_ptr<SelectionText> newline_normalized;
	{
		std::string tmpstr = Document::TransformLineEnds(text->Data(), text->Length(), SC_EOL_LF);
		newline_normalized.reset(new SelectionText());
		newline_normalized->Copy(tmpstr, SC_CP_UTF8, 0, text->rectangular, false);
		text = newline_normalized.get();
	}
#endif

	std::unique_ptr<SelectionText> converted;
	if ((text->codePage != SC_CP_UTF8) && (info == TARGET_UTF8_STRING)) {
		const char *charSet = ::CharacterSetID(text->characterSet);
		if (*charSet) {
			std::string tmputf = ConvertText(text->Data(), text->Length(), "UTF-8", charSet, false);
			converted.reset(new SelectionText());
			converted->Copy(tmputf, SC_CP_UTF8, 0, text->rectangular, false);
			text = converted.get();
		}
	}

	const char *textData = text->Data();
	int len = text->Length();
#if PLAT_GTK_WIN32 == 0
	if (text->rectangular)
		len++;
#endif

	if (info == TARGET_UTF8_STRING) {
		gtk_selection_data_set_text(selection_data, textData, len);
	} else {
		gtk_selection_data_set(selection_data,
			static_cast<GdkAtom>(GDK_SELECTION_TYPE_STRING),
			8, reinterpret_cast<const guchar *>(textData), len);
	}
}

void ScintillaGTK::StoreOnClipboard(SelectionText *clipText) {
	GtkClipboard *clipBoard =
		gtk_widget_get_clipboard(GTK_WIDGET(PWidget(wMain)), atomClipboard);
	if (clipBoard == NULL)
		return;

	if (gtk_clipboard_set_with_data(clipBoard, clipboardCopyTargets, nClipboardCopyTargets,
				    ClipboardGetSelection, ClipboardClearSelection, clipText)) {
		gtk_clipboard_set_can_store(clipBoard, clipboardCopyTargets, nClipboardCopyTargets);
	}
}

void ScintillaGTK::ClipboardGetSelection(GtkClipboard *, GtkSelectionData *selection_data, guint info, void *data) {
	GetSelection(selection_data, info, static_cast<SelectionText*>(data));
}

void ScintillaGTK::ClipboardClearSelection(GtkClipboard *, void *data) {
	SelectionText *obj = static_cast<SelectionText*>(data);
	delete obj;
}

void ScintillaGTK::UnclaimSelection(GdkEventSelection *selection_event) {
	try {
		if (selection_event->selection == GDK_SELECTION_PRIMARY) {
			if (!OwnPrimarySelection()) {
				primary.Clear();
				primarySelection = false;
				FullPaint();
			}
		}
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::PrimarySelection(GtkWidget *, GtkSelectionData *selection_data, guint info, guint, ScintillaGTK *sciThis) {
	try {
		if (SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY) {
			if (sciThis->primary.Empty()) {
				sciThis->CopySelectionRange(&sciThis->primary);
			}
			sciThis->GetSelection(selection_data, info, &sciThis->primary);
		}
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

gboolean ScintillaGTK::PrimaryClear(GtkWidget *widget, GdkEventSelection *event, ScintillaGTK *sciThis) {
	sciThis->UnclaimSelection(event);
	if (GTK_WIDGET_CLASS(sciThis->parentClass)->selection_clear_event) {
		return GTK_WIDGET_CLASS(sciThis->parentClass)->selection_clear_event(widget, event);
	}
	return TRUE;
}

void ScintillaGTK::Resize(int width, int height) {
	int minVScrollBarHeight, minHScrollBarWidth;

#if GTK_CHECK_VERSION(3,0,0)
	GtkRequisition minimum, requisition;
	gtk_widget_get_preferred_size(PWidget(scrollbarv), &minimum, &requisition);
	minVScrollBarHeight = minimum.height;
	verticalScrollBarWidth = requisition.width;
	gtk_widget_get_preferred_size(PWidget(scrollbarh), &minimum, &requisition);
	minHScrollBarWidth = minimum.width;
	horizontalScrollBarHeight = requisition.height;
#else
	minVScrollBarHeight = minHScrollBarWidth = 1;
	verticalScrollBarWidth = GTK_WIDGET(PWidget(scrollbarv))->requisition.width;
	horizontalScrollBarHeight = GTK_WIDGET(PWidget(scrollbarh))->requisition.height;
#endif

	bool showSBHorizontal = horizontalScrollBarVisible && !Wrapping();

	GtkAllocation alloc;
	if (showSBHorizontal) {
		gtk_widget_show(GTK_WIDGET(PWidget(scrollbarh)));
		alloc.x = 0;
		alloc.y = height - horizontalScrollBarHeight;
		alloc.width = std::max(minHScrollBarWidth, width - verticalScrollBarWidth);
		alloc.height = horizontalScrollBarHeight;
		gtk_widget_size_allocate(GTK_WIDGET(PWidget(scrollbarh)), &alloc);
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(scrollbarh)));
		horizontalScrollBarHeight = 0;
	}

	if (verticalScrollBarVisible) {
		gtk_widget_show(GTK_WIDGET(PWidget(scrollbarv)));
		alloc.x = width - verticalScrollBarWidth;
		alloc.y = 0;
		alloc.width = verticalScrollBarWidth;
		alloc.height = std::max(minVScrollBarHeight, height - horizontalScrollBarHeight);
		gtk_widget_size_allocate(GTK_WIDGET(PWidget(scrollbarv)), &alloc);
	} else {
		gtk_widget_hide(GTK_WIDGET(PWidget(scrollbarv)));
		verticalScrollBarWidth = 0;
	}
	if (IS_WIDGET_MAPPED(PWidget(wMain))) {
		ChangeSize();
	}

	alloc.x = 0;
	alloc.y = 0;
	alloc.width = 1;
	alloc.height = 1;
#if GTK_CHECK_VERSION(3, 0, 0)
	gtk_widget_get_preferred_size(PWidget(wText), &requisition, NULL);
	alloc.width = requisition.width;
	alloc.height = requisition.height;
#endif
	alloc.width = std::max(alloc.width, width - verticalScrollBarWidth);
	alloc.height = std::max(alloc.height, height - horizontalScrollBarHeight);
	gtk_widget_size_allocate(GTK_WIDGET(PWidget(wText)), &alloc);
}

static void SetAdjustmentValue(GtkAdjustment *object, int value) {
	GtkAdjustment *adjustment = GTK_ADJUSTMENT(object);
	int maxValue = static_cast<int>(
		gtk_adjustment_get_upper(adjustment) - gtk_adjustment_get_page_size(adjustment));

	if (value > maxValue)
		value = maxValue;
	if (value < 0)
		value = 0;
	gtk_adjustment_set_value(adjustment, value);
}

static int modifierTranslated(int sciModifier) {
	switch (sciModifier) {
		case SCMOD_SHIFT:
			return GDK_SHIFT_MASK;
		case SCMOD_CTRL:
			return GDK_CONTROL_MASK;
		case SCMOD_ALT:
			return GDK_MOD1_MASK;
		case SCMOD_SUPER:
			return GDK_MOD4_MASK;
		default:
			return 0;
	}
}

gint ScintillaGTK::PressThis(GdkEventButton *event) {
	try {
		if (event->type != GDK_BUTTON_PRESS)
			return FALSE;

		if (evbtn) {
			gdk_event_free(evbtn);
		}
		evbtn = gdk_event_copy(reinterpret_cast<GdkEvent *>(event));
		buttonMouse = event->button;
		Point pt;
		pt.x = floor(event->x);
		pt.y = floor(event->y);
		PRectangle rcClient = GetClientRectangle();

		if ((pt.x > rcClient.right) || (pt.y > rcClient.bottom)) {
			return FALSE;
		}

		bool shift = (event->state & GDK_SHIFT_MASK) != 0;
		bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
		bool alt = (event->state & modifierTranslated(rectangularSelectionModifier)) != 0;

		gtk_widget_grab_focus(PWidget(wMain));
		if (event->button == 1) {
#if PLAT_GTK_MACOSX
			bool meta = ctrl;
			ctrl = (event->state & GDK_MOD2_MASK) != 0;
#else
			bool meta = false;
#endif
			ButtonDownWithModifiers(pt, event->time, ModifierFlags(shift, ctrl, alt, meta));
		} else if (event->button == 2) {
			SelectionPosition pos = SPositionFromLocation(pt, false, false, UserVirtualSpace());
			if (OwnPrimarySelection() && primary.Empty())
				CopySelectionRange(&primary);

			sel.Clear();
			SetSelection(pos, pos);
			atomSought = atomUTF8;
			gtk_selection_convert(GTK_WIDGET(PWidget(wMain)), GDK_SELECTION_PRIMARY,
			        atomSought, event->time);
		} else if (event->button == 3) {
			if (!PointInSelection(pt))
				SetEmptySelection(PositionFromLocation(pt));
			if (ShouldDisplayPopup(pt)) {
				int ox = 0;
				int oy = 0;
				gdk_window_get_origin(PWindow(wMain), &ox, &oy);
				ContextMenu(Point(pt.x + ox, pt.y + oy));
			} else {
#if PLAT_GTK_MACOSX
				bool meta = ctrl;
				ctrl = (event->state & GDK_MOD2_MASK) != 0;
#else
				bool meta = false;
#endif
				RightButtonDownWithModifiers(pt, event->time, ModifierFlags(shift, ctrl, alt, meta));
				return FALSE;
			}
		} else if (event->button == 4) {
			if (ctrl)
				SetAdjustmentValue(adjustmenth, xOffset - 6);
			else
				SetAdjustmentValue(adjustmentv, topLine - 3);
		} else if (event->button == 5) {
			if (ctrl)
				SetAdjustmentValue(adjustmenth, xOffset + 6);
			else
				SetAdjustmentValue(adjustmentv, topLine + 3);
		}
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gint ScintillaGTK::Press(GtkWidget *widget, GdkEventButton *event) {
	if (event->window != WindowFromWidget(widget))
		return FALSE;
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->PressThis(event);
}

gint ScintillaGTK::MouseRelease(GtkWidget *widget, GdkEventButton *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		if (!sciThis->HaveMouseCapture())
			return FALSE;
		if (event->button == 1) {
			Point pt;
			pt.x = int(event->x);
			pt.y = int(event->y);
			if (event->window != PWindow(sciThis->wMain))
				pt = sciThis->ptMouseLast;
			const int modifiers = ModifierFlags(
				(event->state & GDK_SHIFT_MASK) != 0,
				(event->state & GDK_CONTROL_MASK) != 0,
				(event->state & modifierTranslated(sciThis->rectangularSelectionModifier)) != 0);
			sciThis->ButtonUpWithModifiers(pt, event->time, modifiers);
		}
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gint ScintillaGTK::ScrollEvent(GtkWidget *widget, GdkEventScroll *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		if (widget == NULL || event == NULL)
			return FALSE;

#if defined(GDK_WINDOWING_WAYLAND)
		if (event->direction == GDK_SCROLL_SMOOTH && GDK_IS_WAYLAND_WINDOW(event->window)) {
			const int smoothScrollFactor = 4;
			sciThis->smoothScrollY += event->delta_y * smoothScrollFactor;
			sciThis->smoothScrollX += event->delta_x * smoothScrollFactor;;
			if (ABS(sciThis->smoothScrollY) >= 1.0) {
				const int scrollLines = trunc(sciThis->smoothScrollY);
				sciThis->ScrollTo(sciThis->topLine + scrollLines);
				sciThis->smoothScrollY -= scrollLines;
			}
			if (ABS(sciThis->smoothScrollX) >= 1.0) {
				const int scrollPixels = trunc(sciThis->smoothScrollX);
				sciThis->HorizontalScrollTo(sciThis->xOffset + scrollPixels);
				sciThis->smoothScrollX -= scrollPixels;
			}
			return TRUE;
		}
#endif

		int cLineScroll;
#if defined(__APPLE__) && !defined(GDK_WINDOWING_QUARTZ)
		cLineScroll = sciThis->linesPerScroll;
		if (cLineScroll == 0)
			cLineScroll = 4;
		sciThis->wheelMouseIntensity = cLineScroll;
#else
		int timeDelta = 1000000;
		GTimeVal curTime;
		g_get_current_time(&curTime);
		if (curTime.tv_sec == sciThis->lastWheelMouseTime.tv_sec)
			timeDelta = curTime.tv_usec - sciThis->lastWheelMouseTime.tv_usec;
		else if (curTime.tv_sec == sciThis->lastWheelMouseTime.tv_sec + 1)
			timeDelta = 1000000 + (curTime.tv_usec - sciThis->lastWheelMouseTime.tv_usec);
		if ((event->direction == sciThis->lastWheelMouseDirection) && (timeDelta < 250000)) {
			if (sciThis->wheelMouseIntensity < 12)
				sciThis->wheelMouseIntensity++;
			cLineScroll = sciThis->wheelMouseIntensity;
		} else {
			cLineScroll = sciThis->linesPerScroll;
			if (cLineScroll == 0)
				cLineScroll = 4;
			sciThis->wheelMouseIntensity = cLineScroll;
		}
#endif
		if (event->direction == GDK_SCROLL_UP || event->direction == GDK_SCROLL_LEFT) {
			cLineScroll *= -1;
		}
		g_get_current_time(&sciThis->lastWheelMouseTime);
		sciThis->lastWheelMouseDirection = event->direction;

		if (event->state & GDK_SHIFT_MASK) {
			return FALSE;
		}

#if GTK_CHECK_VERSION(3,4,0)
		if (event->direction == GDK_SCROLL_SMOOTH) {
			return FALSE;
		}
#endif

		if (event->direction == GDK_SCROLL_LEFT || event->direction == GDK_SCROLL_RIGHT) {
			sciThis->HorizontalScrollTo(sciThis->xOffset + cLineScroll);
		} else if (event->state & GDK_CONTROL_MASK) {
			if (cLineScroll < 0) {
				sciThis->KeyCommand(SCI_ZOOMIN);
			} else {
				sciThis->KeyCommand(SCI_ZOOMOUT);
			}
		} else {
			sciThis->ScrollTo(sciThis->topLine + cLineScroll);
		}
		return TRUE;
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gint ScintillaGTK::Motion(GtkWidget *widget, GdkEventMotion *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		if (event->window != WindowFromWidget(widget))
			return FALSE;
		int x = 0;
		int y = 0;
		GdkModifierType state;
		if (event->is_hint) {
#if GTK_CHECK_VERSION(3,0,0)
			gdk_window_get_device_position(event->window,
				event->device, &x, &y, &state);
#else
			gdk_window_get_pointer(event->window, &x, &y, &state);
#endif
		} else {
			x = static_cast<int>(event->x);
			y = static_cast<int>(event->y);
			state = static_cast<GdkModifierType>(event->state);
		}
		Point pt(x, y);
		const int modifiers = ModifierFlags(
				(event->state & GDK_SHIFT_MASK) != 0,
				(event->state & GDK_CONTROL_MASK) != 0,
				(event->state & modifierTranslated(sciThis->rectangularSelectionModifier)) != 0);
		sciThis->ButtonMoveWithModifiers(pt, event->time, modifiers);
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

static int KeyTranslate(int keyIn) {
	switch (keyIn) {
#if GTK_CHECK_VERSION(3,0,0)
	case GDK_KEY_ISO_Left_Tab:
		return SCK_TAB;
	case GDK_KEY_KP_Down:
		return SCK_DOWN;
	case GDK_KEY_KP_Up:
		return SCK_UP;
	case GDK_KEY_KP_Left:
		return SCK_LEFT;
	case GDK_KEY_KP_Right:
		return SCK_RIGHT;
	case GDK_KEY_KP_Home:
		return SCK_HOME;
	case GDK_KEY_KP_End:
		return SCK_END;
	case GDK_KEY_KP_Page_Up:
		return SCK_PRIOR;
	case GDK_KEY_KP_Page_Down:
		return SCK_NEXT;
	case GDK_KEY_KP_Delete:
		return SCK_DELETE;
	case GDK_KEY_KP_Insert:
		return SCK_INSERT;
	case GDK_KEY_KP_Enter:
		return SCK_RETURN;

	case GDK_KEY_Down:
		return SCK_DOWN;
	case GDK_KEY_Up:
		return SCK_UP;
	case GDK_KEY_Left:
		return SCK_LEFT;
	case GDK_KEY_Right:
		return SCK_RIGHT;
	case GDK_KEY_Home:
		return SCK_HOME;
	case GDK_KEY_End:
		return SCK_END;
	case GDK_KEY_Page_Up:
		return SCK_PRIOR;
	case GDK_KEY_Page_Down:
		return SCK_NEXT;
	case GDK_KEY_Delete:
		return SCK_DELETE;
	case GDK_KEY_Insert:
		return SCK_INSERT;
	case GDK_KEY_Escape:
		return SCK_ESCAPE;
	case GDK_KEY_BackSpace:
		return SCK_BACK;
	case GDK_KEY_Tab:
		return SCK_TAB;
	case GDK_KEY_Return:
		return SCK_RETURN;
	case GDK_KEY_KP_Add:
		return SCK_ADD;
	case GDK_KEY_KP_Subtract:
		return SCK_SUBTRACT;
	case GDK_KEY_KP_Divide:
		return SCK_DIVIDE;
	case GDK_KEY_Super_L:
		return SCK_WIN;
	case GDK_KEY_Super_R:
		return SCK_RWIN;
	case GDK_KEY_Menu:
		return SCK_MENU;

#else

	case GDK_ISO_Left_Tab:
		return SCK_TAB;
	case GDK_KP_Down:
		return SCK_DOWN;
	case GDK_KP_Up:
		return SCK_UP;
	case GDK_KP_Left:
		return SCK_LEFT;
	case GDK_KP_Right:
		return SCK_RIGHT;
	case GDK_KP_Home:
		return SCK_HOME;
	case GDK_KP_End:
		return SCK_END;
	case GDK_KP_Page_Up:
		return SCK_PRIOR;
	case GDK_KP_Page_Down:
		return SCK_NEXT;
	case GDK_KP_Delete:
		return SCK_DELETE;
	case GDK_KP_Insert:
		return SCK_INSERT;
	case GDK_KP_Enter:
		return SCK_RETURN;

	case GDK_Down:
		return SCK_DOWN;
	case GDK_Up:
		return SCK_UP;
	case GDK_Left:
		return SCK_LEFT;
	case GDK_Right:
		return SCK_RIGHT;
	case GDK_Home:
		return SCK_HOME;
	case GDK_End:
		return SCK_END;
	case GDK_Page_Up:
		return SCK_PRIOR;
	case GDK_Page_Down:
		return SCK_NEXT;
	case GDK_Delete:
		return SCK_DELETE;
	case GDK_Insert:
		return SCK_INSERT;
	case GDK_Escape:
		return SCK_ESCAPE;
	case GDK_BackSpace:
		return SCK_BACK;
	case GDK_Tab:
		return SCK_TAB;
	case GDK_Return:
		return SCK_RETURN;
	case GDK_KP_Add:
		return SCK_ADD;
	case GDK_KP_Subtract:
		return SCK_SUBTRACT;
	case GDK_KP_Divide:
		return SCK_DIVIDE;
	case GDK_Super_L:
		return SCK_WIN;
	case GDK_Super_R:
		return SCK_RWIN;
	case GDK_Menu:
		return SCK_MENU;
#endif
	default:
		return keyIn;
	}
}

gboolean ScintillaGTK::KeyThis(GdkEventKey *event) {
	try {
		if (gtk_im_context_filter_keypress(im_context, event)) {
			return 1;
		}
		if (!event->keyval) {
			return true;
		}

		bool shift = (event->state & GDK_SHIFT_MASK) != 0;
		bool ctrl = (event->state & GDK_CONTROL_MASK) != 0;
		bool alt = (event->state & GDK_MOD1_MASK) != 0;
		bool super = (event->state & GDK_MOD4_MASK) != 0;
		guint key = event->keyval;
		if ((ctrl || alt) && (key < 128))
			key = toupper(key);
#if GTK_CHECK_VERSION(3,0,0)
		else if (!ctrl && (key >= GDK_KEY_KP_Multiply && key <= GDK_KEY_KP_9))
#else
		else if (!ctrl && (key >= GDK_KP_Multiply && key <= GDK_KP_9))
#endif
			key &= 0x7F;
		else if (key >= 0xFE00)
			key = KeyTranslate(key);

		bool consumed = false;
#if !(PLAT_GTK_MACOSX)
		bool meta = false;
#else
		bool meta = ctrl;
		ctrl = (event->state & GDK_META_MASK) != 0;
#endif
		bool added = KeyDownWithModifiers(key, ModifierFlags(shift, ctrl, alt, meta, super), &consumed) != 0;
		if (!consumed)
			consumed = added;
		if (event->keyval == 0xffffff && event->length > 0) {
			ClearSelection();
			const int lengthInserted = pdoc->InsertString(CurrentPosition(), event->string, strlen(event->string));
			if (lengthInserted > 0) {
				MovePositionTo(CurrentPosition() + lengthInserted);
			}
		}
		return consumed;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gboolean ScintillaGTK::KeyPress(GtkWidget *widget, GdkEventKey *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->KeyThis(event);
}

gboolean ScintillaGTK::KeyRelease(GtkWidget *widget, GdkEventKey *event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	if (gtk_im_context_filter_keypress(sciThis->im_context, event)) {
		return TRUE;
	}
	return FALSE;
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawPreeditThis(GtkWidget *, cairo_t *cr) {
	try {
		PreEditString pes(im_context);
		PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), pes.str);
		pango_layout_set_attributes(layout, pes.attrs);

		cairo_move_to(cr, 0, 0);
		pango_cairo_show_layout(cr, layout);

		g_object_unref(layout);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gboolean ScintillaGTK::DrawPreedit(GtkWidget *widget, cairo_t *cr, ScintillaGTK *sciThis) {
	return sciThis->DrawPreeditThis(widget, cr);
}

#else

gboolean ScintillaGTK::ExposePreeditThis(GtkWidget *widget, GdkEventExpose *) {
	try {
		PreEditString pes(im_context);
		PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), pes.str);
		pango_layout_set_attributes(layout, pes.attrs);

		cairo_t *context = gdk_cairo_create(reinterpret_cast<GdkDrawable *>(WindowFromWidget(widget)));
		cairo_move_to(context, 0, 0);
		pango_cairo_show_layout(context, layout);
		cairo_destroy(context);
		g_object_unref(layout);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return TRUE;
}

gboolean ScintillaGTK::ExposePreedit(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposePreeditThis(widget, ose);
}

#endif

bool ScintillaGTK::KoreanIME() {
	PreEditString pes(im_context);
	if (pes.pscript != PANGO_SCRIPT_COMMON)
		lastNonCommonScript = pes.pscript;
	return lastNonCommonScript == PANGO_SCRIPT_HANGUL;
}

void ScintillaGTK::MoveImeCarets(int pos) {
	for (size_t r=0; r<sel.Count(); r++) {
		int positionInsert = sel.Range(r).Start().Position();
		sel.Range(r).caret.SetPosition(positionInsert + pos);
		sel.Range(r).anchor.SetPosition(positionInsert + pos);
	}
}

void ScintillaGTK::DrawImeIndicator(int indicator, int len) {
	if (indicator < 8 || indicator > INDIC_MAX) {
		return;
	}
	pdoc->DecorationSetCurrentIndicator(indicator);
	for (size_t r=0; r<sel.Count(); r++) {
		int positionInsert = sel.Range(r).Start().Position();
		pdoc->DecorationFillRange(positionInsert - len, 1, len);
	}
}

static std::vector<int> MapImeIndicators(PangoAttrList *attrs, const char *u8Str) {
	glong charactersLen = g_utf8_strlen(u8Str, strlen(u8Str));
	std::vector<int> indicator(charactersLen, SC_INDICATOR_UNKNOWN);

	PangoAttrIterator *iterunderline = pango_attr_list_get_iterator(attrs);
	if (iterunderline) {
		do {
			PangoAttribute  *attrunderline = pango_attr_iterator_get(iterunderline, PANGO_ATTR_UNDERLINE);
			if (attrunderline) {
				glong start = g_utf8_strlen(u8Str, attrunderline->start_index);
				glong end = g_utf8_strlen(u8Str, attrunderline->end_index);
				PangoUnderline uline = (PangoUnderline)((PangoAttrInt *)attrunderline)->value;
				for (glong i=start; i < end; ++i) {
					switch (uline) {
					case PANGO_UNDERLINE_NONE:
						indicator[i] = SC_INDICATOR_UNKNOWN;
						break;
					case PANGO_UNDERLINE_SINGLE:
						indicator[i] = SC_INDICATOR_INPUT;
						break;
					case PANGO_UNDERLINE_DOUBLE:
					case PANGO_UNDERLINE_LOW:
					case PANGO_UNDERLINE_ERROR:
						break;
					}
				}
			}
		} while (pango_attr_iterator_next(iterunderline));
		pango_attr_iterator_destroy(iterunderline);
	}

	PangoAttrIterator *itercolor = pango_attr_list_get_iterator(attrs);
	if (itercolor) {
		do {
			PangoAttribute  *backcolor = pango_attr_iterator_get(itercolor, PANGO_ATTR_BACKGROUND);
			if (backcolor) {
				glong start = g_utf8_strlen(u8Str, backcolor->start_index);
				glong end = g_utf8_strlen(u8Str, backcolor->end_index);
				for (glong i=start; i < end; ++i) {
					indicator[i] = SC_INDICATOR_TARGET;
				}
			}
		} while (pango_attr_iterator_next(itercolor));
		pango_attr_iterator_destroy(itercolor);
	}
	return indicator;
}

void ScintillaGTK::SetCandidateWindowPos() {
	Point pt = PointMainCaret();
	GdkRectangle imeBox = {0};
	imeBox.x = pt.x;
	imeBox.y = pt.y + vs.lineHeight;
	gtk_im_context_set_cursor_location(im_context, &imeBox);
}

void ScintillaGTK::CommitThis(char *commitStr) {
	try {
		view.imeCaretBlockOverride = false;

		if (pdoc->TentativeActive()) {
			pdoc->TentativeUndo();
		}

		const char *charSetSource = CharacterSetID();

		glong uniStrLen = 0;
		gunichar *uniStr = g_utf8_to_ucs4_fast(commitStr, strlen(commitStr), &uniStrLen);
		for (glong i = 0; i < uniStrLen; i++) {
			gchar u8Char[UTF8MaxBytes+2] = {0};
			gint u8CharLen = g_unichar_to_utf8(uniStr[i], u8Char);
			std::string docChar = u8Char;
			if (!IsUnicodeMode())
				docChar = ConvertText(u8Char, u8CharLen, charSetSource, "UTF-8", true);

			AddCharUTF(docChar.c_str(), docChar.size());
		}
		g_free(uniStr);
		ShowCaretAtCurrentPosition();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::Commit(GtkIMContext *, char  *str, ScintillaGTK *sciThis) {
	sciThis->CommitThis(str);
}

void ScintillaGTK::PreeditChangedInlineThis() {
	try {
		if (pdoc->IsReadOnly() || SelectionContainsProtected()) {
			gtk_im_context_reset(im_context);
			return;
		}

		view.imeCaretBlockOverride = false;

		bool initialCompose = false;
		if (pdoc->TentativeActive()) {
			pdoc->TentativeUndo();
		} else {
			initialCompose = true;
		}

		PreEditString preeditStr(im_context);
		const char *charSetSource = CharacterSetID();

		if (!preeditStr.validUTF8 || (charSetSource == NULL)) {
			ShowCaretAtCurrentPosition();
			return;
		}

		if (preeditStr.uniStrLen == 0 || preeditStr.uniStrLen > maxLenInputIME) {
			ShowCaretAtCurrentPosition();
			return;
		}

		if (initialCompose)
			ClearBeforeTentativeStart();
		pdoc->TentativeStart();

		std::vector<int> indicator = MapImeIndicators(preeditStr.attrs, preeditStr.str);

		bool tmpRecordingMacro = recordingMacro;
		recordingMacro = false;
		for (glong i = 0; i < preeditStr.uniStrLen; i++) {
			gchar u8Char[UTF8MaxBytes+2] = {0};
			gint u8CharLen = g_unichar_to_utf8(preeditStr.uniStr[i], u8Char);
			std::string docChar = u8Char;
			if (!IsUnicodeMode())
				docChar = ConvertText(u8Char, u8CharLen, charSetSource, "UTF-8", true);

			AddCharUTF(docChar.c_str(), docChar.size());

			DrawImeIndicator(indicator[i], docChar.size());
		}
		recordingMacro = tmpRecordingMacro;

		int imeEndToImeCaretU32 = preeditStr.cursor_pos - preeditStr.uniStrLen;
		int imeCaretPosDoc = pdoc->GetRelativePosition(CurrentPosition(), imeEndToImeCaretU32);

		MoveImeCarets(- CurrentPosition() + imeCaretPosDoc);

		if (KoreanIME()) {
#if !PLAT_GTK_WIN32
			if (preeditStr.cursor_pos > 0) {
				int oneCharBefore = pdoc->GetRelativePosition(CurrentPosition(), -1);
				MoveImeCarets(- CurrentPosition() + oneCharBefore);
			}
#endif
			view.imeCaretBlockOverride = true;
		}

		EnsureCaretVisible();
		SetCandidateWindowPos();
		ShowCaretAtCurrentPosition();
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::PreeditChangedWindowedThis() {
	try {
		PreEditString pes(im_context);
		if (strlen(pes.str) > 0) {
			PangoLayout *layout = gtk_widget_create_pango_layout(PWidget(wText), pes.str);
			pango_layout_set_attributes(layout, pes.attrs);

			gint w, h;
			pango_layout_get_pixel_size(layout, &w, &h);
			g_object_unref(layout);

			gint x, y;
			gdk_window_get_origin(PWindow(wText), &x, &y);

			Point pt = PointMainCaret();
			if (pt.x < 0)
				pt.x = 0;
			if (pt.y < 0)
				pt.y = 0;

			gtk_window_move(GTK_WINDOW(PWidget(wPreedit)), x + pt.x, y + pt.y);
			gtk_window_resize(GTK_WINDOW(PWidget(wPreedit)), w, h);
			gtk_widget_show(PWidget(wPreedit));
			gtk_widget_queue_draw_area(PWidget(wPreeditDraw), 0, 0, w, h);
		} else {
			gtk_widget_hide(PWidget(wPreedit));
		}
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::PreeditChanged(GtkIMContext *, ScintillaGTK *sciThis) {
	if ((sciThis->imeInteraction == imeInline) || (sciThis->KoreanIME())) {
		sciThis->PreeditChangedInlineThis();
	} else {
		sciThis->PreeditChangedWindowedThis();
	}
}

void ScintillaGTK::StyleSetText(GtkWidget *widget, GtkStyle *, void*) {
	RealizeText(widget, NULL);
}

void ScintillaGTK::RealizeText(GtkWidget *widget, void*) {
	if (WindowFromWidget(widget)) {
#if GTK_CHECK_VERSION(3,22,0)
#elif GTK_CHECK_VERSION(3,0,0)
		gdk_window_set_background_pattern(WindowFromWidget(widget), NULL);
#else
		gdk_window_set_back_pixmap(WindowFromWidget(widget), NULL, FALSE);
#endif
	}
}

static GObjectClass *scintilla_class_parent_class;

void ScintillaGTK::Dispose(GObject *object) {
	try {
		ScintillaObject *scio = SCINTILLA(object);
		ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(scio->pscin);

		if (PWidget(sciThis->scrollbarv)) {
			gtk_widget_unparent(PWidget(sciThis->scrollbarv));
			sciThis->scrollbarv = NULL;
		}

		if (PWidget(sciThis->scrollbarh)) {
			gtk_widget_unparent(PWidget(sciThis->scrollbarh));
			sciThis->scrollbarh = NULL;
		}

		scintilla_class_parent_class->dispose(object);
	} catch (...) {
	}
}

void ScintillaGTK::Destroy(GObject *object) {
	try {
		ScintillaObject *scio = SCINTILLA(object);

		if (!scio->pscin)
			return;
		ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(scio->pscin);
		sciThis->Finalise();

		delete sciThis;
		scio->pscin = 0;
		scintilla_class_parent_class->finalize(object);
	} catch (...) {
	}
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawTextThis(cairo_t *cr) {
	try {
		paintState = painting;
		repaintFullWindow = false;

		rcPaint = GetClientRectangle();

		PLATFORM_ASSERT(rgnUpdate == NULL);
		rgnUpdate = cairo_copy_clip_rectangle_list(cr);
		if (rgnUpdate && rgnUpdate->status != CAIRO_STATUS_SUCCESS) {
			fprintf(stderr, "DrawTextThis failed to copy update region %d [%d]\n", rgnUpdate->status, rgnUpdate->num_rectangles);
			cairo_rectangle_list_destroy(rgnUpdate);
			rgnUpdate = 0;
		}

		double x1, y1, x2, y2;
		cairo_clip_extents(cr, &x1, &y1, &x2, &y2);
		rcPaint.left = x1;
		rcPaint.top = y1;
		rcPaint.right = x2;
		rcPaint.bottom = y2;
		PRectangle rcClient = GetClientRectangle();
		paintingAllText = rcPaint.Contains(rcClient);
		std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(SC_TECHNOLOGY_DEFAULT));
		surfaceWindow->Init(cr, PWidget(wText));
		Paint(surfaceWindow.get(), rcPaint);
		surfaceWindow->Release();
		if ((paintState == paintAbandoned) || repaintFullWindow) {
			FullPaint();
		}
		paintState = notPainting;
		repaintFullWindow = false;

		if (rgnUpdate) {
			cairo_rectangle_list_destroy(rgnUpdate);
		}
		rgnUpdate = 0;
		paintState = notPainting;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}

	return FALSE;
}

gboolean ScintillaGTK::DrawText(GtkWidget *, cairo_t *cr, ScintillaGTK *sciThis) {
	return sciThis->DrawTextThis(cr);
}

gboolean ScintillaGTK::DrawThis(cairo_t *cr) {
	try {
#ifdef GTK_STYLE_CLASS_SCROLLBARS_JUNCTION
		if (verticalScrollBarVisible && horizontalScrollBarVisible && !Wrapping()) {
			GtkStyleContext *styleContext = gtk_widget_get_style_context(PWidget(wMain));
			PRectangle rc = GetClientRectangle();

			gtk_style_context_save(styleContext);
			gtk_style_context_add_class(styleContext, GTK_STYLE_CLASS_SCROLLBARS_JUNCTION);

			gtk_render_background(styleContext, cr, rc.right, rc.bottom,
					verticalScrollBarWidth, horizontalScrollBarHeight);
			gtk_render_frame(styleContext, cr, rc.right, rc.bottom,
					verticalScrollBarWidth, horizontalScrollBarHeight);

			gtk_style_context_restore(styleContext);
		}
#endif

		gtk_container_propagate_draw(
		    GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarh), cr);
		gtk_container_propagate_draw(
		    GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarv), cr);
#if GTK_CHECK_VERSION(3,0,0)
		if (gtk_check_version(3,9,2) == NULL) {
			gtk_container_propagate_draw(
					GTK_CONTAINER(PWidget(wMain)), PWidget(wText), cr);
		}
#endif
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gboolean ScintillaGTK::DrawMain(GtkWidget *widget, cairo_t *cr) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->DrawThis(cr);
}

#else

gboolean ScintillaGTK::ExposeTextThis(GtkWidget * /*widget*/, GdkEventExpose *ose) {
	try {
		paintState = painting;

		rcPaint.left = ose->area.x;
		rcPaint.top = ose->area.y;
		rcPaint.right = ose->area.x + ose->area.width;
		rcPaint.bottom = ose->area.y + ose->area.height;

		PLATFORM_ASSERT(rgnUpdate == NULL);
		rgnUpdate = gdk_region_copy(ose->region);
		PRectangle rcClient = GetClientRectangle();
		paintingAllText = rcPaint.Contains(rcClient);
		std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(SC_TECHNOLOGY_DEFAULT));
		cairo_t *cr = gdk_cairo_create(PWindow(wText));
		surfaceWindow->Init(cr, PWidget(wText));
		Paint(surfaceWindow.get(), rcPaint);
		surfaceWindow->Release();
		cairo_destroy(cr);
		if ((paintState == paintAbandoned) || repaintFullWindow) {
			FullPaint();
		}
		paintState = notPainting;
		repaintFullWindow = false;

		if (rgnUpdate) {
			gdk_region_destroy(rgnUpdate);
		}
		rgnUpdate = 0;
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}

	return FALSE;
}

gboolean ScintillaGTK::ExposeText(GtkWidget *widget, GdkEventExpose *ose, ScintillaGTK *sciThis) {
	return sciThis->ExposeTextThis(widget, ose);
}

gboolean ScintillaGTK::ExposeMain(GtkWidget *widget, GdkEventExpose *ose) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->Expose(widget, ose);
}

gboolean ScintillaGTK::Expose(GtkWidget *, GdkEventExpose *ose) {
	try {
		gtk_container_propagate_expose(
		    GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarh), ose);
		gtk_container_propagate_expose(
		    GTK_CONTAINER(PWidget(wMain)), PWidget(scrollbarv), ose);

	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

#endif

void ScintillaGTK::ScrollSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
		sciThis->ScrollTo(static_cast<int>(gtk_adjustment_get_value(adj)), false);
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::ScrollHSignal(GtkAdjustment *adj, ScintillaGTK *sciThis) {
	try {
		sciThis->HorizontalScrollTo(static_cast<int>(gtk_adjustment_get_value(adj)));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::SelectionReceived(GtkWidget *widget,
                                     GtkSelectionData *selection_data, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->ReceivedSelection(selection_data);
}

void ScintillaGTK::SelectionGet(GtkWidget *widget,
                                GtkSelectionData *selection_data, guint info, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		if (SelectionOfGSD(selection_data) == GDK_SELECTION_PRIMARY) {
			if (sciThis->primary.Empty()) {
				sciThis->CopySelectionRange(&sciThis->primary);
			}
			sciThis->GetSelection(selection_data, info, &sciThis->primary);
		}
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

gint ScintillaGTK::SelectionClear(GtkWidget *widget, GdkEventSelection *selection_event) {
	ScintillaGTK *sciThis = FromWidget(widget);
	sciThis->UnclaimSelection(selection_event);
	if (GTK_WIDGET_CLASS(sciThis->parentClass)->selection_clear_event) {
		return GTK_WIDGET_CLASS(sciThis->parentClass)->selection_clear_event(widget, selection_event);
	}
	return TRUE;
}

gboolean ScintillaGTK::DragMotionThis(GdkDragContext *context,
                                 gint x, gint y, guint dragtime) {
	try {
		Point npt(x, y);
		SetDragPosition(SPositionFromLocation(npt, false, false, UserVirtualSpace()));
		GdkDragAction preferredAction = gdk_drag_context_get_suggested_action(context);
		GdkDragAction actions = gdk_drag_context_get_actions(context);
		SelectionPosition pos = SPositionFromLocation(npt);
		if ((inDragDrop == ddDragging) && (PositionInSelection(pos.Position()))) {
			preferredAction = static_cast<GdkDragAction>(0);
		} else if (actions == actionCopyOrMove) {
			preferredAction = GDK_ACTION_MOVE;
		}
		gdk_drag_status(context, preferredAction, dragtime);
	} catch (...) {
		errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

gboolean ScintillaGTK::DragMotion(GtkWidget *widget, GdkDragContext *context,
                                 gint x, gint y, guint dragtime) {
	ScintillaGTK *sciThis = FromWidget(widget);
	return sciThis->DragMotionThis(context, x, y, dragtime);
}

void ScintillaGTK::DragLeave(GtkWidget *widget, GdkDragContext * /*context*/, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::DragEnd(GtkWidget *widget, GdkDragContext * /*context*/) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		if (!sciThis->dragWasDropped)
			sciThis->SetEmptySelection(sciThis->posDrag);
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
		sciThis->inDragDrop = ddNone;
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

gboolean ScintillaGTK::Drop(GtkWidget *widget, GdkDragContext * /*context*/,
                            gint, gint, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
	return FALSE;
}

void ScintillaGTK::DragDataReceived(GtkWidget *widget, GdkDragContext * /*context*/,
                                    gint, gint, GtkSelectionData *selection_data, guint /*info*/, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		sciThis->ReceivedDrop(selection_data);
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

void ScintillaGTK::DragDataGet(GtkWidget *widget, GdkDragContext *context,
                               GtkSelectionData *selection_data, guint info, guint) {
	ScintillaGTK *sciThis = FromWidget(widget);
	try {
		sciThis->dragWasDropped = true;
		if (!sciThis->sel.Empty()) {
			sciThis->GetSelection(selection_data, info, &sciThis->drag);
		}
		GdkDragAction action = gdk_drag_context_get_selected_action(context);
		if (action == GDK_ACTION_MOVE) {
			for (size_t r=0; r<sciThis->sel.Count(); r++) {
				if (sciThis->posDrop >= sciThis->sel.Range(r).Start()) {
					if (sciThis->posDrop > sciThis->sel.Range(r).End()) {
						sciThis->posDrop.Add(-sciThis->sel.Range(r).Length());
					} else {
						sciThis->posDrop.Add(-SelectionRange(sciThis->posDrop, sciThis->sel.Range(r).Start()).Length());
					}
				}
			}
			sciThis->ClearSelection();
		}
		sciThis->SetDragPosition(SelectionPosition(Sci::invalidPosition));
	} catch (...) {
		sciThis->errorStatus = SC_STATUS_FAILURE;
	}
}

int ScintillaGTK::TimeOut(gpointer ptt) {
	TimeThunk *tt = static_cast<TimeThunk *>(ptt);
	tt->scintilla->TickFor(tt->reason);
	return 1;
}

gboolean ScintillaGTK::IdleCallback(gpointer pSci) {
	ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(pSci);
	bool ret = sciThis->Idle();
	if (ret == false) {
		sciThis->SetIdle(false);
	}
	return ret;
}

gboolean ScintillaGTK::StyleIdle(gpointer pSci) {
	ScintillaGTK *sciThis = static_cast<ScintillaGTK *>(pSci);
	sciThis->IdleWork();
	return FALSE;
}

void ScintillaGTK::IdleWork() {
	Editor::IdleWork();
	styleIdleID = 0;
}

void ScintillaGTK::QueueIdleWork(WorkNeeded::workItems items, Sci::Position upTo) {
	Editor::QueueIdleWork(items, upTo);
	if (!styleIdleID) {
		styleIdleID = gdk_threads_add_idle_full(G_PRIORITY_HIGH_IDLE, StyleIdle, this, NULL);
	}
}

void ScintillaGTK::SetDocPointer(Document *document) {
	Editor::SetDocPointer(document);
}

void ScintillaGTK::PopUpCB(GtkMenuItem *menuItem, ScintillaGTK *sciThis) {
	guint action = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(menuItem), "CmdNum"));
	if (action) {
		sciThis->Command(action);
	}
}

gboolean ScintillaGTK::PressCT(GtkWidget *widget, GdkEventButton *event, ScintillaGTK *sciThis) {
	try {
		if (event->window != WindowFromWidget(widget))
			return FALSE;
		if (event->type != GDK_BUTTON_PRESS)
			return FALSE;
		Point pt;
		pt.x = int(event->x);
		pt.y = int(event->y);
		sciThis->ct.MouseClick(pt);
		sciThis->CallTipClick();
	} catch (...) {
	}
	return TRUE;
}

#if GTK_CHECK_VERSION(3,0,0)

gboolean ScintillaGTK::DrawCT(GtkWidget *widget, cairo_t *cr, CallTip *ctip) {
	try {
		std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(SC_TECHNOLOGY_DEFAULT));
		surfaceWindow->Init(cr, widget);
		surfaceWindow->SetUnicodeMode(SC_CP_UTF8 == ctip->codePage);
		surfaceWindow->SetDBCSMode(ctip->codePage);
		ctip->PaintCT(surfaceWindow.get());
		surfaceWindow->Release();
	} catch (...) {
	}
	return TRUE;
}

#else

gboolean ScintillaGTK::ExposeCT(GtkWidget *widget, GdkEventExpose * /*ose*/, CallTip *ctip) {
	try {
		std::unique_ptr<Surface> surfaceWindow(Surface::Allocate(SC_TECHNOLOGY_DEFAULT));
		cairo_t *cr = gdk_cairo_create(WindowFromWidget(widget));
		surfaceWindow->Init(cr, widget);
		surfaceWindow->SetUnicodeMode(SC_CP_UTF8 == ctip->codePage);
		surfaceWindow->SetDBCSMode(ctip->codePage);
		ctip->PaintCT(surfaceWindow.get());
		surfaceWindow->Release();
		cairo_destroy(cr);
	} catch (...) {
	}
	return TRUE;
}

#endif

AtkObject* ScintillaGTK::GetAccessibleThis(GtkWidget */*widget*/) {
	return NULL;
}

AtkObject* ScintillaGTK::GetAccessible(GtkWidget */*widget*/) {
	return NULL;
}

sptr_t ScintillaGTK::DirectFunction(
    sptr_t ptr, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return reinterpret_cast<ScintillaGTK *>(ptr)->WndProc(iMessage, wParam, lParam);
}

sptr_t scintilla_send_message(ScintillaObject *sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	ScintillaGTK *psci = static_cast<ScintillaGTK *>(sci->pscin);
	return psci->WndProc(iMessage, wParam, lParam);
}

gintptr scintilla_object_send_message(ScintillaObject *sci, unsigned int iMessage, uptr_t wParam, sptr_t lParam) {
	return scintilla_send_message(sci, iMessage, wParam, lParam);
}

static void scintilla_class_init(ScintillaClass *klass);
static void scintilla_init(ScintillaObject *sci);

extern void Platform_Initialise();
extern void Platform_Finalise();

GType scintilla_get_type() {
        static GType scintilla_type = 0;
        try {

                if (!scintilla_type) {
                        scintilla_type = g_type_from_name("ScintillaObject");
                        if (!scintilla_type) {
                                GTypeQuery query;
                                g_type_query(GTK_TYPE_CONTAINER, &query);

                                guint16 dynamic_class_size = query.class_size + 64;
                                guint16 dynamic_instance_size = query.instance_size + 32;

                                GTypeInfo scintilla_info = {
                                        dynamic_class_size,
                                        NULL,
                                        NULL,
                                        (GClassInitFunc) scintilla_class_init,
                                        NULL,
                                        NULL,
                                        dynamic_instance_size,
                                        0,
                                        (GInstanceInitFunc) scintilla_init,
                                        NULL
                                };
                                scintilla_type = g_type_register_static(
                                            GTK_TYPE_CONTAINER, "ScintillaObject", &scintilla_info, (GTypeFlags) 0);
                        }
                }

	} catch (...) {
	}
	return scintilla_type;
}

extern "C" GType scintilla_object_get_type() {
	return scintilla_get_type();
}

void ScintillaGTK::ClassInit(OBJECT_CLASS* object_class, GtkWidgetClass *widget_class, GtkContainerClass *container_class) {
	Platform_Initialise();
#ifdef SCI_LEXER
	Scintilla_LinkLexers();
#endif
	atomClipboard = gdk_atom_intern("CLIPBOARD", FALSE);
	atomUTF8 = gdk_atom_intern("UTF8_STRING", FALSE);
	atomString = GDK_SELECTION_TYPE_STRING;
	atomUriList = gdk_atom_intern("text/uri-list", FALSE);
	atomDROPFILES_DND = gdk_atom_intern("DROPFILES_DND", FALSE);

	object_class->dispose = Dispose;
	object_class->finalize = Destroy;
#if GTK_CHECK_VERSION(3,0,0)
	widget_class->get_preferred_width = GetPreferredWidth;
	widget_class->get_preferred_height = GetPreferredHeight;
#else
	widget_class->size_request = SizeRequest;
#endif
	widget_class->size_allocate = SizeAllocate;
#if GTK_CHECK_VERSION(3,0,0)
	widget_class->draw = DrawMain;
#else
	widget_class->expose_event = ExposeMain;
#endif
	widget_class->motion_notify_event = Motion;
	widget_class->button_press_event = Press;
	widget_class->button_release_event = MouseRelease;
	widget_class->scroll_event = ScrollEvent;
	widget_class->key_press_event = KeyPress;
	widget_class->key_release_event = KeyRelease;
	widget_class->focus_in_event = FocusIn;
	widget_class->focus_out_event = FocusOut;
	widget_class->selection_received = SelectionReceived;
	widget_class->selection_get = SelectionGet;
	widget_class->selection_clear_event = SelectionClear;

	widget_class->drag_data_received = DragDataReceived;
	widget_class->drag_motion = DragMotion;
	widget_class->drag_leave = DragLeave;
	widget_class->drag_end = DragEnd;
	widget_class->drag_drop = Drop;
	widget_class->drag_data_get = DragDataGet;

	widget_class->realize = Realize;
	widget_class->unrealize = UnRealize;
	widget_class->map = Map;
	widget_class->unmap = UnMap;

	widget_class->get_accessible = GetAccessible;

	container_class->forall = MainForAll;
}

static void scintilla_class_init(ScintillaClass *klass) {
	try {
		OBJECT_CLASS *object_class = (OBJECT_CLASS*) klass;
		GtkWidgetClass *widget_class = (GtkWidgetClass*) klass;
		GtkContainerClass *container_class = (GtkContainerClass*) klass;

		GSignalFlags sigflags = GSignalFlags(G_SIGNAL_ACTION | G_SIGNAL_RUN_LAST);
		scintilla_signals[COMMAND_SIGNAL] = g_signal_new(
		            "command",
		            G_TYPE_FROM_CLASS(object_class),
		            sigflags,
		            G_STRUCT_OFFSET(ScintillaClass, command),
		            NULL,
		            NULL,
		            scintilla_marshal_VOID__INT_OBJECT,
		            G_TYPE_NONE,
		            2, G_TYPE_INT, GTK_TYPE_WIDGET);

		scintilla_signals[NOTIFY_SIGNAL] = g_signal_new(
		            SCINTILLA_NOTIFY,
		            G_TYPE_FROM_CLASS(object_class),
		            sigflags,
		            G_STRUCT_OFFSET(ScintillaClass, notify),
		            NULL,
		            NULL,
		            scintilla_marshal_VOID__INT_BOXED,
		            G_TYPE_NONE,
		            2, G_TYPE_INT, SCINTILLA_TYPE_NOTIFICATION);

		klass->command = NULL;
		klass->notify = NULL;
		scintilla_class_parent_class = G_OBJECT_CLASS(g_type_class_peek_parent(klass));
		ScintillaGTK::ClassInit(object_class, widget_class, container_class);
	} catch (...) {
	}
}

static void scintilla_init(ScintillaObject *sci) {
	try {
		gtk_widget_set_can_focus(GTK_WIDGET(sci), TRUE);
		sci->pscin = new ScintillaGTK(sci);
	} catch (...) {
	}
}

GtkWidget* scintilla_new() {
	GtkWidget *widget = GTK_WIDGET(g_object_new(scintilla_get_type(), NULL));
	gtk_widget_set_direction(widget, GTK_TEXT_DIR_LTR);

	return widget;
}

extern "C" GtkWidget *scintilla_object_new() {
	return scintilla_new();
}

void scintilla_set_id(ScintillaObject *sci, uptr_t id) {
	ScintillaGTK *psci = static_cast<ScintillaGTK *>(sci->pscin);
	psci->ctrlID = id;
}

void scintilla_release_resources(void) {
	try {
		Platform_Finalise();
	} catch (...) {
	}
}

static void *copy_(void *src) { return src; }
static void free_(void *) { }

GType scnotification_get_type(void) {
	static gsize type_id = 0;
	if (g_once_init_enter(&type_id)) {
		gsize id = (gsize) g_boxed_type_register_static(
		                            g_intern_static_string("SCNotification"),
		                            (GBoxedCopyFunc) copy_,
		                            (GBoxedFreeFunc) free_);
		g_once_init_leave(&type_id, id);
	}
	return (GType) type_id;
}