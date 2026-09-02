//------------------------------------------------------------------------
// ForTran - custom VSTGUI controls implementation
//------------------------------------------------------------------------

#include "ForTranControls.h"

#include <algorithm>
#include <cmath>

using namespace VSTGUI;

namespace ForTran {

namespace {
constexpr double kPI = 3.14159265358979323846;

/** The DXi's rotary sweep, in degrees: CVMRotaryFaderCtrl's
    SetRange(30, 329, FALSE). */
constexpr double kKnobMinDegrees = 30.0;
constexpr double kKnobMaxDegrees = 329.0;

/** Pixels of vertical drag for the full range. */
constexpr CCoord kDragRange = 150.;

/** Breathing room left at the foot of every vertical slider.
 
    The .rc heights put both groups of sliders within about two pixels of the
    label painted underneath them - the harmonic columns end 1.5 px above the
    numbers 1..10, and the filter sliders 1.6 px above "Cutoff Q Poles". The
    original got away with it because a Windows trackbar draws a thin channel
    and a small thumb; a drawn slider with a full-width handle sits right on
    the lettering.
 
    Rather than shortening each group by a different hand-picked amount, the
    control keeps its .rc rect - so the click target is unchanged - and draws
    into a slightly shorter one. */
constexpr CCoord kSliderBottomInset = 7.;

inline float dragValue (float startValue, CCoord dy, bool fine)
{
	const double range = fine ? kDragRange * 5. : kDragRange;
	return static_cast<float> (std::clamp (startValue - dy / range, 0.0, 1.0));
}
} // anonymous namespace

//------------------------------------------------------------------------
// FtKnob
//------------------------------------------------------------------------
FtKnob::FtKnob (const CRect& size, IControlListener* listener, int32_t tag,
                CBitmap* scale, CBitmap* knob, CBitmap* dot)
: CControl (size, listener, tag, nullptr)
, mScale (scale), mKnob (knob), mDot (dot)
{
	setWantsFocus (true);
}

//------------------------------------------------------------------------
void FtKnob::draw (CDrawContext* context)
{
	const CRect r = getViewSize ();

	if (mScale)
		mScale->draw (context, CRect (r.left, r.top,
		                              r.left + mScale->getWidth (),
		                              r.top + mScale->getHeight ()));

	CPoint centre (r.left + (mScale ? mScale->getWidth () : r.getWidth ()) / 2.,
	               r.top  + (mScale ? mScale->getHeight () : r.getHeight ()) / 2.);

	if (mKnob)
	{
		const CCoord kw = mKnob->getWidth (), kh = mKnob->getHeight ();
		mKnob->draw (context, CRect (centre.x - kw / 2., centre.y - kh / 2.,
		                             centre.x + kw / 2., centre.y + kh / 2.));
	}

	if (mDot && mKnob)
	{
		// radius = knobWidth/2 - dotWidth/2 - 3, from SetBitmap().
		const CCoord radius = mKnob->getWidth () / 2. - mDot->getWidth () / 2. - 3.;

		const double pos     = kKnobMinDegrees +
		                       getValueNormalized () * (kKnobMaxDegrees - kKnobMinDegrees);
		const double degrees = 360.0 - pos;
		const double rad     = degrees * kPI / 180.0;

		const CCoord dw = mDot->getWidth (), dh = mDot->getHeight ();
		const CCoord x  = centre.x - dw / 2. + radius * std::sin (rad);
		const CCoord y  = centre.y - dh / 2. + radius * std::cos (rad);
		mDot->draw (context, CRect (x, y, x + dw, y + dh));
	}

	setDirty (false);
}

//------------------------------------------------------------------------
void FtKnob::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	mDragging = true;
	mStartValue = getValueNormalized ();
	mStartPoint = event.mousePosition;
	beginEdit ();
	event.consumed = true;
}

void FtKnob::onMouseMoveEvent (MouseMoveEvent& event)
{
	if (!mDragging)
		return;
	setValueNormalized (dragValue (mStartValue,
	                               event.mousePosition.y - mStartPoint.y,
	                               event.modifiers.has (ModifierKey::Shift)));
	valueChanged ();
	invalid ();
	event.consumed = true;
}

void FtKnob::onMouseUpEvent (MouseUpEvent& event)
{
	if (!mDragging)
		return;
	mDragging = false;
	endEdit ();
	event.consumed = true;
}

void FtKnob::onMouseCancelEvent (MouseCancelEvent& event)
{
	if (mDragging) { mDragging = false; endEdit (); }
	event.consumed = true;
}

void FtKnob::onMouseEnterEvent (MouseEnterEvent& event)
{
	if (mHover)
		mHover->controlHovered (this, true);
	event.consumed = true;
}

void FtKnob::onMouseExitEvent (MouseExitEvent& event)
{
	if (mHover)
		mHover->controlHovered (this, false);
	event.consumed = true;
}

void FtKnob::onMouseWheelEvent (MouseWheelEvent& event)
{
	const float step = event.modifiers.has (ModifierKey::Shift) ? 0.002f : 0.02f;
	beginEdit ();
	setValueNormalized (std::clamp (getValueNormalized () +
	                                static_cast<float> (event.deltaY) * step, 0.f, 1.f));
	valueChanged ();
	endEdit ();
	invalid ();
	event.consumed = true;
}

//------------------------------------------------------------------------
// FtVSlider
//------------------------------------------------------------------------
FtVSlider::FtVSlider (const CRect& size, IControlListener* listener, int32_t tag)
: CControl (size, listener, tag, nullptr)
{
	setWantsFocus (true);
}

//------------------------------------------------------------------------
float FtVSlider::valueFromPoint (const CPoint& where) const
{
	const CRect r = getViewSize ();
	const CCoord travel = r.getHeight () - kSliderBottomInset - 12.;
	if (travel <= 0.)
		return 0.f;
	// Top is maximum, as the DXi's property page read its trackbars:
	// fVal = (max - GetPos()) / scale.
	const double v = 1.0 - ((where.y - r.top - 6.) / travel);
	return static_cast<float> (std::clamp (v, 0.0, 1.0));
}

//------------------------------------------------------------------------
void FtVSlider::draw (CDrawContext* context)
{
	const CRect r = getViewSize ();
	const CCoord cx     = r.left + r.getWidth () / 2.;
	const CCoord bottom = r.bottom - kSliderBottomInset;

	context->setDrawMode (kAntiAliasing);

	// groove
	context->setFillColor (CColor (0, 0, 0, 150));
	context->drawRect (CRect (cx - 2., r.top + 4., cx + 2., bottom - 4.), kDrawFilled);

	// handle
	const CCoord travel = r.getHeight () - kSliderBottomInset - 12.;
	const CCoord y = r.top + 6. + (1.0 - getValueNormalized ()) * travel;
	CRect handle (r.left + 1., y - 5., r.right - 1., y + 5.);

	context->setFillColor (CColor (225, 230, 235, 255));
	context->setFrameColor (CColor (0, 0, 0, 200));
	context->setLineWidth (1.);
	context->drawRect (handle, kDrawFilledAndStroked);

	// a centre line on the handle, so its position reads at a glance
	context->setFrameColor (CColor (90, 95, 100, 255));
	context->drawLine (CPoint (handle.left + 2., y), CPoint (handle.right - 2., y));

	setDirty (false);
}

//------------------------------------------------------------------------
void FtVSlider::setMuteable (float restore)
{
	mMuteable = true;
	mRestore = (restore > 0.f) ? restore : 1.f;
}

void FtVSlider::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;

	// Alt-click mutes, and alt-click again brings the level back. Shift is
	// already taken by the fine drag below, and this has to be a modifier
	// rather than a new control: there is nowhere on the panel to put ten
	// more buttons, and ten more parameters would be a deviation from the
	// DXi's table that a saved project could not survive.
	if (mMuteable && event.modifiers.has (ModifierKey::Alt))
	{
		const float now = getValueNormalized ();
		beginEdit ();
		if (now > 0.f)
		{
			mRestore = now;
			setValueNormalized (0.f);
		}
		else
		{
			setValueNormalized (mRestore);
		}
		valueChanged ();
		endEdit ();
		invalid ();
		event.consumed = true;
		return;                       // deliberately no drag from this click
	}

	mDragging = true;
	mStartPoint = event.mousePosition;
	beginEdit ();
	if (!event.modifiers.has (ModifierKey::Shift))
	{
		setValueNormalized (valueFromPoint (event.mousePosition));
		valueChanged ();
		invalid ();
	}
	mStartValue = getValueNormalized ();
	event.consumed = true;
}

void FtVSlider::onMouseMoveEvent (MouseMoveEvent& event)
{
	if (!mDragging)
		return;
	if (event.modifiers.has (ModifierKey::Shift))
		setValueNormalized (dragValue (mStartValue,
		                               event.mousePosition.y - mStartPoint.y, true));
	else
		setValueNormalized (valueFromPoint (event.mousePosition));
	valueChanged ();
	invalid ();
	event.consumed = true;
}

void FtVSlider::onMouseUpEvent (MouseUpEvent& event)
{
	if (!mDragging)
		return;
	mDragging = false;
	endEdit ();
	event.consumed = true;
}

void FtVSlider::onMouseCancelEvent (MouseCancelEvent& event)
{
	if (mDragging) { mDragging = false; endEdit (); }
	event.consumed = true;
}

void FtVSlider::onMouseEnterEvent (MouseEnterEvent& event)
{
	if (mHover)
		mHover->controlHovered (this, true);
	event.consumed = true;
}

void FtVSlider::onMouseExitEvent (MouseExitEvent& event)
{
	if (mHover)
		mHover->controlHovered (this, false);
	event.consumed = true;
}

void FtVSlider::onMouseWheelEvent (MouseWheelEvent& event)
{
	const float step = event.modifiers.has (ModifierKey::Shift) ? 0.002f : 0.02f;
	beginEdit ();
	setValueNormalized (std::clamp (getValueNormalized () +
	                                static_cast<float> (event.deltaY) * step, 0.f, 1.f));
	valueChanged ();
	endEdit ();
	invalid ();
	event.consumed = true;
}

//------------------------------------------------------------------------
// FtToggle
//------------------------------------------------------------------------
FtToggle::FtToggle (const CRect& size, IControlListener* listener, int32_t tag,
                    CBitmap* off, CBitmap* on)
: CControl (size, listener, tag, nullptr)
, mOff (off), mOn (on)
{
}

void FtToggle::draw (CDrawContext* context)
{
	CBitmap* bmp = (getValueNormalized () >= 0.5f) ? mOn : mOff;
	if (bmp)
		bmp->draw (context, getViewSize ());
	setDirty (false);
}

void FtToggle::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	const bool on = getValueNormalized () >= 0.5f;
	if (mRadio && on)
	{
		event.consumed = true;      // a radio button cannot switch itself off
		return;
	}
	beginEdit ();
	setValueNormalized (on ? 0.f : 1.f);
	valueChanged ();
	endEdit ();
	invalid ();
	event.consumed = true;
}

//------------------------------------------------------------------------
// FtTextStepper
//------------------------------------------------------------------------
FtTextStepper::FtTextStepper (const CRect& size, IControlListener* listener, int32_t tag,
                              int32_t numSteps, TextProvider provider)
: CControl (size, listener, tag, nullptr)
, mNumSteps (std::max<int32_t> (1, numSteps))
, mProvider (std::move (provider))
{
}

int32_t FtTextStepper::currentStep () const
{
	return static_cast<int32_t> (getValueNormalized () * mNumSteps + 0.5f);
}

void FtTextStepper::setStep (int32_t step)
{
	step = std::clamp<int32_t> (step, 0, mNumSteps);
	setValueNormalized (static_cast<float> (step) / static_cast<float> (mNumSteps));
	valueChanged ();
	invalid ();
}

std::string FtTextStepper::currentText () const
{
	return mProvider ? mProvider (currentStep ()) : std::string ();
}

void FtTextStepper::draw (CDrawContext* context)
{
	const CRect r = getViewSize ();

	// A plate, not bare text. These four sit on the pale left-hand panel and
	// were unreadable drawn straight onto it. The plate fills the whole
	// control rather than being sized to the string, as the value readouts
	// are: the text cycles Off / Add / Mult / Max, and a plate that changed
	// width on every click would jump about.
	CRect plate (r);
	plate.inset (0.5, 0.5);            // so the 1 px border lands on the grid
	context->setDrawMode (kAliasing);
	context->setLineWidth (1.);
	context->setFillColor (mPlateColor);
	context->setFrameColor (mBorderColor);
	context->drawRect (plate, kDrawFilledAndStroked);

	const std::string text = currentText ();
	if (!text.empty ())
	{
		context->setFont (kNormalFontSmall);
		context->setFontColor (mTextColor);
		context->drawString (text.c_str (), r, kCenterText, true);
	}
	setDirty (false);
}

void FtTextStepper::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	mDragging = true;
	mMoved = false;
	mStartPoint = event.mousePosition;
	mStartStep = currentStep ();
	beginEdit ();
	event.consumed = true;
}

void FtTextStepper::onMouseMoveEvent (MouseMoveEvent& event)
{
	if (!mDragging)
		return;
	const CCoord dy = mStartPoint.y - event.mousePosition.y;
	if (std::fabs (dy) > 3.)
	{
		mMoved = true;
		setStep (mStartStep + static_cast<int32_t> (dy / 8.));
	}
	event.consumed = true;
}

void FtTextStepper::onMouseUpEvent (MouseUpEvent& event)
{
	if (!mDragging)
		return;
	mDragging = false;
	if (!mMoved)                       // a plain click advances one step
		setStep ((currentStep () + 1) > mNumSteps ? 0 : currentStep () + 1);
	endEdit ();
	event.consumed = true;
}

void FtTextStepper::onMouseWheelEvent (MouseWheelEvent& event)
{
	beginEdit ();
	setStep (currentStep () + ((event.deltaY > 0.) ? 1 : -1));
	endEdit ();
	event.consumed = true;
}

//------------------------------------------------------------------------
// FtCurveView
//------------------------------------------------------------------------
FtCurveView::FtCurveView (const CRect& size, const CColor& trace)
: CView (size)
, mTrace (trace)
{
	setMouseEnabled (false);
}

//------------------------------------------------------------------------
void FtCurveView::setData (const float* data, int count, bool bipolar)
{
	mBipolar = bipolar;
	mData.assign (data, data + std::max (0, count));
	invalid ();
}

//------------------------------------------------------------------------
void FtCurveView::setFixedScale (float fullScale)
{
	if (mFixedScale == fullScale)
		return;
	mFixedScale = fullScale;
	invalid ();
}

//------------------------------------------------------------------------
void FtCurveView::setCaption (const std::string& caption)
{
	if (mCaption == caption)
		return;
	mCaption = caption;
	invalid ();
}

//------------------------------------------------------------------------
void FtCurveView::setAnnotation (const std::string& annotation)
{
	if (mAnnotation == annotation)
		return;
	mAnnotation = annotation;
	invalid ();
}

//------------------------------------------------------------------------
void FtCurveView::draw (CDrawContext* context)
{
	const CRect r = getViewSize ();

	context->setDrawMode (kAntiAliasing);
	context->setFillColor (CColor (0, 0, 0, 190));
	context->setFrameColor (CColor (255, 255, 255, 110));
	context->setLineWidth (1.);
	CRect plate (r);
	plate.inset (0.5, 0.5);
	context->drawRect (plate, kDrawFilledAndStroked);

	// Labels first, so the trace draws over them rather than under.
	if (!mCaption.empty () || !mAnnotation.empty ())
	{
		context->setFont (kNormalFontVerySmall);
		CRect text (r);
		text.inset (4., 3.);
		text.bottom = text.top + 11.;
		if (!mCaption.empty ())
		{
			context->setFontColor (CColor (255, 255, 255, 200));
			context->drawString (mCaption.c_str (), text, kLeftText, true);
		}
		if (!mAnnotation.empty ())
		{
			context->setFontColor (CColor (255, 255, 255, 140));
			context->drawString (mAnnotation.c_str (), text, kRightText, true);
		}
	}

	if (mData.size () < 2)
	{
		setDirty (false);
		return;
	}

	// Scale to fit. The response and waveform panels can hold values well
	// outside 0..1 - summed magnitudes, or ten harmonics stacked up - so
	// they normalise rather than clipping. The envelope panels set a fixed
	// scale instead, because their height is the Level control and must not
	// be normalised away.
	float peak = mFixedScale;
	if (peak <= 0.f)
	{
		for (float v : mData)
			if (std::isfinite (v))
				peak = std::max (peak, std::fabs (v));
	}
	if (peak <= 0.f)
		peak = 1.f;

	const CCoord left = r.left + 2., width = r.getWidth () - 4.;

	// The labels get a band of their own at the top. Without it a full-level
	// trace runs through the lettering - and a disabled envelope, which is a
	// flat 1.0, disappears into the panel's own top border.
	const CCoord labelBand = (mCaption.empty () && mAnnotation.empty ()) ? 0. : 12.;
	const CCoord top    = r.top + 2. + labelBand;
	const CCoord height = r.getHeight () - 4. - labelBand;

	if (mBipolar)
	{
		context->setFrameColor (CColor (255, 255, 255, 60));
		const CCoord mid = top + height / 2.;
		context->drawLine (CPoint (left, mid), CPoint (left + width, mid));
	}

	auto pointAt = [&] (size_t i)
	{
		const double x = left + width * (double) i / (double) (mData.size () - 1);
		double v = std::isfinite (mData[i]) ? mData[i] / peak : 0.0;
		// A fixed scale can be exceeded; keep the trace inside the panel.
		if (v > 1.0)  v = 1.0;
		if (v < -1.0) v = -1.0;
		const double y = mBipolar ? (top + height / 2. - v * (height / 2. - 1.))
		                          : (top + height - v * height);
		return CPoint (x, y);
	};

	context->setFrameColor (mTrace);
	context->setLineWidth (1.);
	CPoint prev = pointAt (0);
	for (size_t i = 1; i < mData.size (); ++i)
	{
		const CPoint p = pointAt (i);
		context->drawLine (prev, p);
		prev = p;
	}

	setDirty (false);
}

//------------------------------------------------------------------------
// FtValueDisplay
//------------------------------------------------------------------------
namespace { constexpr CCoord kPlatePadX = 3.; }

FtValueDisplay::FtValueDisplay (const CRect& size)
: CView (size)
{
	setMouseEnabled (false);
	setTransparency (true);
}

void FtValueDisplay::setText (const std::string& text)
{
	if (text == mText)
		return;
	mText = text;
	invalid ();
}

void FtValueDisplay::draw (CDrawContext* context)
{
	if (!mText.empty ())
	{
		const CRect r = getViewSize ();
		context->setFont (kNormalFontVerySmall);
		const CCoord textWidth = context->getStringWidth (mText.c_str ());

		CCoord plateWidth = textWidth + 2 * kPlatePadX;
		if (plateWidth > r.getWidth ())
			plateWidth = r.getWidth ();

		// Centre on the anchor when one is set, otherwise on the view.
		CCoord left = (mAnchorX >= 0.) ? (mAnchorX - plateWidth / 2.)
		                               : (r.left + (r.getWidth () - plateWidth) / 2.);
		left = std::max (r.left, std::min (left, r.right - plateWidth));

		CRect plate (left, r.top, left + plateWidth, r.bottom);
		plate.inset (0.5, 0.5);

		context->setDrawMode (kAliasing);
		context->setLineWidth (1.);
		context->setFillColor (mPlate);
		context->setFrameColor (mBorder);
		context->drawRect (plate, kDrawFilledAndStroked);

		context->setFontColor (mColor);
		context->drawString (mText.c_str (), CRect (left, r.top, left + plateWidth, r.bottom),
		                     kCenterText, true);
	}
	setDirty (false);
}

//------------------------------------------------------------------------
// FtLabel
//------------------------------------------------------------------------
FtLabel::FtLabel (const CRect& size, const std::string& text,
                  CHoriTxtAlign align, CColor colour)
: CView (size)
, mText (text)
, mAlign (align)
, mColour (colour)
{
	setMouseEnabled (false);       // never in the way of a knob beside it
}

void FtLabel::draw (CDrawContext* context)
{
	context->setFont (kNormalFontVerySmall);
	context->setFontColor (mColour);
	context->drawString (mText.c_str (), getViewSize (), mAlign, true);
	setDirty (false);
}

//------------------------------------------------------------------------
// FtTextButton
//------------------------------------------------------------------------
FtTextButton::FtTextButton (const CRect& size, const std::string& text,
                            std::function<void ()> onClick)
: CView (size)
, mText (text)
, mOnClick (std::move (onClick))
{
	setMouseEnabled (true);
	setWantsFocus (false);
}

void FtTextButton::setText (const std::string& text)
{
	if (mText == text)
		return;
	mText = text;
	invalid ();
}

void FtTextButton::draw (CDrawContext* context)
{
	CRect r = getViewSize ();
	r.inset (0.5, 0.5);

	context->setDrawMode (kAliasing);
	context->setLineWidth (1.);
	context->setFillColor (mHot ? CColor (40, 40, 40, 255) : CColor (0, 0, 0, 255));
	context->setFrameColor (mHot ? CColor (255, 255, 255, 255) : CColor (255, 255, 255, 160));
	context->drawRect (r, kDrawFilledAndStroked);

	context->setFont (kNormalFontVerySmall);
	context->setFontColor (CColor (255, 255, 255, 255));
	context->drawString (mText.c_str (), getViewSize (), kCenterText, true);

	setDirty (false);
}

void FtTextButton::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	event.consumed = true;
	if (mOnClick)
		mOnClick ();          // may delete nothing here, but see the note in
		                      // ForTranEditor: the callback only toggles
		                      // visibility, it never destroys this view.
}

void FtTextButton::onMouseEnterEvent (MouseEnterEvent& event)
{
	mHot = true;
	invalid ();
	event.consumed = true;
}

void FtTextButton::onMouseExitEvent (MouseExitEvent& event)
{
	mHot = false;
	invalid ();
	event.consumed = true;
}

//------------------------------------------------------------------------
// FtPanel
//------------------------------------------------------------------------
const CCoord FtPanel::kTitleHeight = 22.;

FtPanel::FtPanel (const CRect& size, const std::string& title)
: CViewContainer (size)
, mTitle (title)
{
	// Opaque, so nothing of the window shows through the panel. The plate is
	// painted in drawBackgroundRect; this is what the container reports about
	// itself, and a transparent one makes the frame treat the panel as a hole.
	setBackgroundColor (CColor (12, 16, 20, 255));
	setMouseEnabled (true);
}

void FtPanel::drawBackgroundRect (CDrawContext* context, const CRect&)
{
	// Local coordinates: the context is already translated by the origin.
	CRect r (0., 0., getViewSize ().getWidth (), getViewSize ().getHeight ());

	context->setDrawMode (kAntiAliasing);
	context->setLineWidth (1.);

	// A drop shadow, so the panel reads as being above the window rather
	// than cut into it. Drawn first, offset down and right.
	CRect shadow (r);
	shadow.offset (3., 3.);
	context->setFillColor (CColor (0, 0, 0, 80));
	context->drawRect (shadow, kDrawFilled);

	CRect plate (r);
	plate.inset (0.5, 0.5);
	context->setFillColor (CColor (18, 22, 28, 255));
	context->setFrameColor (CColor (255, 255, 255, 190));
	context->drawRect (plate, kDrawFilledAndStroked);

	// The title bar: a lighter band with a rule under it, so the strip you
	// can drag the panel by is the strip that looks draggable.
	CRect bar (plate);
	bar.bottom = bar.top + kTitleHeight;
	context->setFillColor (CColor (44, 52, 62, 255));
	context->drawRect (bar, kDrawFilled);
	context->setFrameColor (CColor (255, 255, 255, 90));
	context->drawLine (CPoint (bar.left, bar.bottom), CPoint (bar.right, bar.bottom));

	if (!mTitle.empty ())
	{
		CRect t (bar);
		t.inset (8., 0.);
		context->setFont (kNormalFontSmall);
		context->setFontColor (CColor (255, 255, 255, 230));
		context->drawString (mTitle.c_str (), t, kLeftText, true);
	}

	// An inner hairline, which is what makes it look like a raised panel
	// rather than a rectangle of paint.
	CRect inner (plate);
	inner.inset (1., 1.);
	inner.top = bar.bottom + 1.;
	context->setFrameColor (CColor (255, 255, 255, 30));
	context->drawRect (inner, kDrawStroked);
}

void FtPanel::moveBy (const CPoint& delta)
{
	if (delta.x == 0. && delta.y == 0.)
		return;

	CRect r (getViewSize ());
	r.offset (delta.x, delta.y);

	// Clamped whole, not just far enough to keep the title bar reachable:
	// the panel is smaller than the window in both directions, so there is
	// no reason to let any of it hang off the edge.
	if (auto* f = getFrame ())
	{
		const CCoord maxX = f->getViewSize ().getWidth () - r.getWidth ();
		const CCoord maxY = f->getViewSize ().getHeight () - r.getHeight ();
		if (r.left < 0.) r.offset (-r.left, 0.);
		if (r.top  < 0.) r.offset (0., -r.top);
		if (r.left > maxX) r.offset (maxX - r.left, 0.);
		if (r.top  > maxY) r.offset (0., maxY - r.top);
	}

	if (r == getViewSize ())
		return;

	// The old rectangle has to be repainted as well as the new one, or the
	// panel smears across the window as it is dragged.
	invalid ();
	setViewSize (r);
	setMouseableArea (r);
	invalid ();
	if (auto* f = getFrame ())
		f->invalid ();
}

//------------------------------------------------------------------------
// FtPanelGrip
//------------------------------------------------------------------------
FtPanelGrip::FtPanelGrip (const CRect& size, FtPanel* panel)
: CView (size)
, mPanel (panel)
{
	setMouseEnabled (true);
}

void FtPanelGrip::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft () || !mPanel)
		return;
	mDragging = true;
	// The position arrives in the PANEL's coordinates, so this is the grab
	// point within the panel. Every later event is measured against the
	// panel's current origin, so the difference is the movement since the
	// last one however far the panel has already travelled.
	mGrab = event.mousePosition;
	event.consumed = true;
}

void FtPanelGrip::onMouseMoveEvent (MouseMoveEvent& event)
{
	if (!mDragging || !mPanel)
		return;
	mPanel->moveBy (CPoint (event.mousePosition.x - mGrab.x,
	                        event.mousePosition.y - mGrab.y));
	event.consumed = true;
}

void FtPanelGrip::onMouseUpEvent (MouseUpEvent& event)
{
	if (!mDragging)
		return;
	mDragging = false;
	event.consumed = true;
}

void FtPanelGrip::onMouseCancelEvent (MouseCancelEvent& event)
{
	mDragging = false;
	event.consumed = true;
}

//------------------------------------------------------------------------
// FtScrim
//------------------------------------------------------------------------
FtScrim::FtScrim (const CRect& size, std::function<void ()> onClick)
: CView (size)
, mOnClick (std::move (onClick))
{
	setMouseEnabled (true);
}

void FtScrim::draw (CDrawContext* context)
{
	// A wash rather than nothing, so it is obvious which panel has the
	// pointer and the controls behind are visibly out of play.
	context->setDrawMode (kAliasing);
	context->setFillColor (CColor (0, 0, 0, 90));
	context->drawRect (getViewSize (), kDrawFilled);
	setDirty (false);
}

void FtScrim::onMouseDownEvent (MouseDownEvent& event)
{
	event.consumed = true;
	if (mOnClick)
		mOnClick ();
}

//------------------------------------------------------------------------
} // namespace ForTran
