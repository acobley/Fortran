//------------------------------------------------------------------------
// ForTran - custom VSTGUI controls
//
// The DXi's property page used four kinds of control:
//
//   * CVMRotaryFaderCtrl  - a bitmap rotary knob, 29 x 29 px, drawn from
//     rotaryscale / rotaryknob / rotarydot. Twenty-nine of them.
//   * CMySliderControl    - a plain Windows trackbar (msctls_trackbar32)
//     drawn transparently. Thirteen, all vertical. There is no slider
//     artwork in the DXi's resources, so this port draws its own.
//   * CButtonST           - a push-like checkbox showing Green.bmp or
//     Red.bmp, 22 x 22 px. Twenty-two of them.
//   * NewSpin             - an owner-drawn text cycler, for the four
//     filter-shape selectors.
//
// Remember that the .rc geometry is fiction for the knobs and the buttons:
// they resize themselves at runtime and only x, y survive. See
// PORTING-NOTES section 3.
//------------------------------------------------------------------------

#pragma once

#include "vstgui/vstgui.h"

#include <functional>
#include <string>
#include <vector>

namespace ForTran {

//------------------------------------------------------------------------
/** Told when the pointer enters or leaves a control, so the editor can put
    a value readout beside whatever is being touched.

    The panel has no room for 42 permanent readouts - 17 of the controls have
    nowhere to put one without covering the painted lettering - so the
    readout is transient and shared. See PORTING-NOTES section 3. */
struct IHoverListener
{
	virtual ~IHoverListener () = default;
	virtual void controlHovered (VSTGUI::CControl* control, bool entered) = 0;
};

//------------------------------------------------------------------------
/** Bitmap rotary knob.

    The DXi's sweep is SetRange(30, 329) in DEGREES, with
        degree = 360 - pos
        dot    = centre + radius * (sin(degree), cos(degree))
    so the travel runs lower-left -> left -> top -> right -> lower-right:
    299 degrees clockwise with the dead zone at the bottom, which is what a
    knob normally does. */
class FtKnob : public VSTGUI::CControl
{
public:
	FtKnob (const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag,
	        VSTGUI::CBitmap* scale, VSTGUI::CBitmap* knob, VSTGUI::CBitmap* dot);

	void draw (VSTGUI::CDrawContext* context) override;

	void onMouseDownEvent (VSTGUI::MouseDownEvent& event) override;
	void onMouseMoveEvent (VSTGUI::MouseMoveEvent& event) override;
	void onMouseUpEvent (VSTGUI::MouseUpEvent& event) override;
	void onMouseCancelEvent (VSTGUI::MouseCancelEvent& event) override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

	void onMouseEnterEvent (VSTGUI::MouseEnterEvent& event) override;
	void onMouseExitEvent (VSTGUI::MouseExitEvent& event) override;
	void setHoverListener (IHoverListener* listener) { mHover = listener; }

	CLASS_METHODS (FtKnob, VSTGUI::CControl)

private:
	VSTGUI::SharedPointer<VSTGUI::CBitmap> mScale, mKnob, mDot;
	IHoverListener* mHover = nullptr;
	bool  mDragging = false;
	float mStartValue = 0.f;
	VSTGUI::CPoint mStartPoint;
};

//------------------------------------------------------------------------
/** Vertical slider, drawn rather than bitmapped - the DXi used a native
    Windows trackbar and shipped no slider artwork.

    Top is maximum, as the DXi's property page read it. */
class FtVSlider : public VSTGUI::CControl
{
public:
	FtVSlider (const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag);

	void draw (VSTGUI::CDrawContext* context) override;

	void onMouseDownEvent (VSTGUI::MouseDownEvent& event) override;
	void onMouseMoveEvent (VSTGUI::MouseMoveEvent& event) override;
	void onMouseUpEvent (VSTGUI::MouseUpEvent& event) override;
	void onMouseCancelEvent (VSTGUI::MouseCancelEvent& event) override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

	void onMouseEnterEvent (VSTGUI::MouseEnterEvent& event) override;
	void onMouseExitEvent (VSTGUI::MouseExitEvent& event) override;
	void setHoverListener (IHoverListener* listener) { mHover = listener; }

	/** Turn on alt-click mute for this slider, restoring to `restore` when a
	    muted slider has nothing else to go back to.

	    ForTran has no per-harmonic on/off switch. The buttons under the
	    sliders are INVERT - they multiply the level by -1 (ForTran.cpp:789)
	    and never silence anything - so the only way to take a partial out is
	    to run its slider to the bottom, which loses where it was. Alt-click
	    remembers. Off by default: on the cutoff, gain and pole sliders
	    "muted" would mean nothing. */
	void setMuteable (float restore);

	CLASS_METHODS (FtVSlider, VSTGUI::CControl)

private:
	float valueFromPoint (const VSTGUI::CPoint& where) const;

	IHoverListener* mHover = nullptr;
	bool  mDragging = false;
	float mStartValue = 0.f;
	bool  mMuteable = false;
	float mRestore = 1.f;
	VSTGUI::CPoint mStartPoint;
};

//------------------------------------------------------------------------
/** Two-bitmap toggle. GREEN IS ON: every one of these is BS_AUTOCHECKBOX
    over a CButtonST built with SetBitmaps(IDB_GREEN, NULL, IDB_RED, NULL),
    and the checked branch of DrawTheBitmap (BtnST.cpp:750) takes the first
    of that pair. This comment previously claimed the opposite and the two
    PNGs were built to match it; see PORTING-NOTES section 3. */
class FtToggle : public VSTGUI::CControl
{
public:
	FtToggle (const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag,
	          VSTGUI::CBitmap* off, VSTGUI::CBitmap* on);

	void draw (VSTGUI::CDrawContext* context) override;
	void onMouseDownEvent (VSTGUI::MouseDownEvent& event) override;

	/** A toggle used as a radio button - the ten formant selectors - stays
	    lit and does not turn itself off when clicked again. */
	void setRadio (bool radio) { mRadio = radio; }

	CLASS_METHODS (FtToggle, VSTGUI::CControl)

private:
	VSTGUI::SharedPointer<VSTGUI::CBitmap> mOff, mOn;
	bool mRadio = false;
};

//------------------------------------------------------------------------
/** Stepped text readout - the stand-in for the DXi's NewSpin. Click to
    advance, drag vertically, or use the wheel.

    Drawn as white text on a black plate with a hairline white border: these
    sit on the pale left-hand panel and bare text on it cannot be read. */
class FtTextStepper : public VSTGUI::CControl
{
public:
	using TextProvider = std::function<std::string (int step)>;

	FtTextStepper (const VSTGUI::CRect& size, VSTGUI::IControlListener* listener, int32_t tag,
	               int32_t numSteps, TextProvider provider);

	void setTextColor (const VSTGUI::CColor& c)   { mTextColor = c; invalid (); }
	void setPlateColor (const VSTGUI::CColor& c)  { mPlateColor = c; invalid (); }
	void setBorderColor (const VSTGUI::CColor& c) { mBorderColor = c; invalid (); }

	std::string currentText () const;

	void draw (VSTGUI::CDrawContext* context) override;
	void onMouseDownEvent (VSTGUI::MouseDownEvent& event) override;
	void onMouseMoveEvent (VSTGUI::MouseMoveEvent& event) override;
	void onMouseUpEvent (VSTGUI::MouseUpEvent& event) override;
	void onMouseWheelEvent (VSTGUI::MouseWheelEvent& event) override;

	CLASS_METHODS (FtTextStepper, VSTGUI::CControl)

private:
	void setStep (int32_t step);
	int32_t currentStep () const;

	int32_t      mNumSteps = 1;
	TextProvider mProvider;
	VSTGUI::CColor mTextColor   = VSTGUI::CColor (255, 255, 255, 255);
	VSTGUI::CColor mPlateColor  = VSTGUI::CColor (0, 0, 0, 255);
	VSTGUI::CColor mBorderColor = VSTGUI::CColor (255, 255, 255, 255);
	bool           mDragging = false, mMoved = false;
	VSTGUI::CPoint mStartPoint;
	int32_t        mStartStep = 0;
};

//------------------------------------------------------------------------
/** A curve panel - the stand-in for the DXi's DrawArea. Draws a polyline
    over a dark plate, with a baseline.

    Two of these: the filter response across the 552 bins, and one cycle of
    the waveform. Both are mouse-disabled. */
class FtCurveView : public VSTGUI::CView
{
public:
	FtCurveView (const VSTGUI::CRect& size, const VSTGUI::CColor& trace);

	/** By default the data is rescaled to fill the panel; the response curve
	    wants it (magnitudes can sum above 1), the waveform wants it too so a
	    quiet patch is still visible. */
	void setData (const float* data, int count, bool bipolar);

	/** Draw against a fixed full-scale value instead of rescaling. The
	    envelope panels need this: an envelope's HEIGHT is meaningful - it is
	    what the Level control sets - and normalising would draw every level
	    setting identically. Zero restores rescaling. */
	void setFixedScale (float fullScale);

	/** Small labels inside the panel: what it shows, top left, and a figure
	    for it, top right. Either may be empty. */
	void setCaption (const std::string& caption);
	void setAnnotation (const std::string& annotation);

	void draw (VSTGUI::CDrawContext* context) override;

	CLASS_METHODS (FtCurveView, VSTGUI::CView)

private:
	std::vector<float> mData;
	bool           mBipolar = false;
	float          mFixedScale = 0.f;
	std::string    mCaption;
	std::string    mAnnotation;
	VSTGUI::CColor mTrace;
};

//------------------------------------------------------------------------
/** Plain text, no plate and no border. The LFO panel paints its own dark
    ground, so its column numbers and row names want lettering and nothing
    else - a plate behind each one would be ten small boxes of noise. */
class FtLabel : public VSTGUI::CView
{
public:
	FtLabel (const VSTGUI::CRect& size, const std::string& text,
	         VSTGUI::CHoriTxtAlign align = VSTGUI::kCenterText,
	         VSTGUI::CColor colour = VSTGUI::CColor (255, 255, 255, 200));

	void draw (VSTGUI::CDrawContext* context) override;

	CLASS_METHODS (FtLabel, VSTGUI::CView)

private:
	std::string mText;
	VSTGUI::CHoriTxtAlign mAlign;
	VSTGUI::CColor mColour;
};

//------------------------------------------------------------------------
/** A plain clickable label: white text on a black plate with a hairline
    white border, the same treatment the readouts and the filter selectors
    use. Not a parameter - it calls back, and is used to open and close the
    harmonic LFO panel. */
class FtTextButton : public VSTGUI::CView
{
public:
	FtTextButton (const VSTGUI::CRect& size, const std::string& text,
	              std::function<void ()> onClick);

	void setText (const std::string& text);

	void draw (VSTGUI::CDrawContext* context) override;
	void onMouseDownEvent (VSTGUI::MouseDownEvent& event) override;
	void onMouseEnterEvent (VSTGUI::MouseEnterEvent& event) override;
	void onMouseExitEvent (VSTGUI::MouseExitEvent& event) override;

	CLASS_METHODS (FtTextButton, VSTGUI::CView)

private:
	std::string mText;
	std::function<void ()> mOnClick;
	bool mHot = false;
};

//------------------------------------------------------------------------
/** The panel the harmonic LFO knobs live on: a plate with a title, drawn
    behind whatever is added to it.

    A container rather than a real window. A separate platform window would
    have to be parented, positioned, kept on top and torn down by hand on
    three hosts' terms; a child of the frame cannot be misplaced, cannot be
    left behind when the editor closes, and draws in the same coordinate
    space as everything else. */
class FtPanel : public VSTGUI::CViewContainer
{
public:
	FtPanel (const VSTGUI::CRect& size, const std::string& title);

	/** NOT draw(). CViewContainer::drawRect paints its background and then
	    its children and never calls draw() at all, so a draw() override on a
	    container is dead code that compiles - which is exactly how this
	    panel first shipped with no plate and no border.

	    The context is already translated by the container's origin when this
	    is called, so everything here is in LOCAL coordinates from (0,0). */
	void drawBackgroundRect (VSTGUI::CDrawContext* context,
	                         const VSTGUI::CRect& updateRect) override;

	/** Shift the panel, clamped so it cannot be dragged off the window. */
	void moveBy (const VSTGUI::CPoint& delta);

	/** The title bar's height. The grip covers exactly this strip. */
	static const VSTGUI::CCoord kTitleHeight;

	CLASS_METHODS (FtPanel, VSTGUI::CViewContainer)

private:
	std::string mTitle;
};

//------------------------------------------------------------------------
/** The title bar, and the thing you drag the panel by.

    A child view rather than logic on the container itself: VSTGUI routes the
    move and up events of a drag to whichever view consumed the mouse-down,
    and for a container that bookkeeping is the container's own - a leaf
    child gets the same straightforward treatment a slider does. */
class FtPanelGrip : public VSTGUI::CView
{
public:
	FtPanelGrip (const VSTGUI::CRect& size, FtPanel* panel);

	void onMouseDownEvent (VSTGUI::MouseDownEvent& event) override;
	void onMouseMoveEvent (VSTGUI::MouseMoveEvent& event) override;
	void onMouseUpEvent (VSTGUI::MouseUpEvent& event) override;
	void onMouseCancelEvent (VSTGUI::MouseCancelEvent& event) override;

	void draw (VSTGUI::CDrawContext*) override {}    // the panel paints it

	CLASS_METHODS (FtPanelGrip, VSTGUI::CView)

private:
	FtPanel* mPanel = nullptr;
	bool mDragging = false;
	VSTGUI::CPoint mGrab;
};

//------------------------------------------------------------------------
/** A transparent sheet the width and height of the window, sitting under a
    panel and over everything else.

    It does two jobs at once: a click anywhere outside the panel lands here
    and closes it, and while the panel is up nothing behind it can be
    dragged by accident. */
class FtScrim : public VSTGUI::CView
{
public:
	FtScrim (const VSTGUI::CRect& size, std::function<void ()> onClick);

	void draw (VSTGUI::CDrawContext* context) override;
	void onMouseDownEvent (VSTGUI::MouseDownEvent& event) override;

	CLASS_METHODS (FtScrim, VSTGUI::CView)

private:
	std::function<void ()> mOnClick;
};

//------------------------------------------------------------------------
/** A small always-visible value readout, as SpaceDub ended up with: text on
    an opaque plate sized to the string, with a hairline border. */
class FtValueDisplay : public VSTGUI::CView
{
public:
	explicit FtValueDisplay (const VSTGUI::CRect& size);

	void setText (const std::string& text);

	/** Centre the plate on this x, in frame coordinates, clamped so it stays
	    inside the view. The view itself spans the full window width, so a
	    reading beside a control at the very edge slides along rather than
	    being dragged away from it - which is what happened when the view was
	    narrow and got clamped as a whole. */
	void setAnchorX (VSTGUI::CCoord x) { mAnchorX = x; invalid (); }

	void draw (VSTGUI::CDrawContext* context) override;

	CLASS_METHODS (FtValueDisplay, VSTGUI::CView)

private:
	std::string    mText;
	VSTGUI::CCoord mAnchorX = -1.;
	VSTGUI::CColor mColor  = VSTGUI::CColor (255, 255, 255, 255);
	VSTGUI::CColor mPlate  = VSTGUI::CColor (0, 0, 0, 255);
	VSTGUI::CColor mBorder = VSTGUI::CColor (255, 255, 255, 255);
};

//------------------------------------------------------------------------
} // namespace ForTran
