//------------------------------------------------------------------------
// ForTran - the Polivoks panel's controls
//------------------------------------------------------------------------

#include "PvControls.h"

#include <algorithm>
#include <cmath>

using namespace VSTGUI;

namespace ForTran {

namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr double kDragRange = 200.;   // px of vertical drag for the full sweep

/** Degrees from 12 o'clock, clockwise - the mock-up's pol(). */
CPoint pol (const CPoint& c, double r, double deg)
{
	const double t = deg * kPi / 180.;
	return CPoint (c.x + r * std::sin (t), c.y - r * std::cos (t));
}

CColor hex (uint32_t rgb, double a = 1.)
{
	return CColor (static_cast<uint8_t> (rgb >> 16), static_cast<uint8_t> ((rgb >> 8) & 0xff),
	               static_cast<uint8_t> (rgb & 0xff), static_cast<uint8_t> (255 * a));
}

void line (CDrawContext* c, const CPoint& a, const CPoint& b, const CColor& col, double w)
{
	c->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
	c->setLineWidth (w);
	c->setFrameColor (col);
	c->drawLine (a, b);
}

void disc (CDrawContext* c, const CPoint& centre, double r, const CColor& fill)
{
	c->setFillColor (fill);
	c->drawEllipse (CRect (centre.x - r, centre.y - r, centre.x + r, centre.y + r), kDrawFilled);
}

void ring (CDrawContext* c, const CPoint& centre, double r, const CColor& col, double w)
{
	c->setLineStyle (kLineSolid);
	c->setLineWidth (w);
	c->setFrameColor (col);
	c->drawEllipse (CRect (centre.x - r, centre.y - r, centre.x + r, centre.y + r), kDrawStroked);
}

/** The mock-up's "capgrad": a black knob top lit from the upper left. */
void capFill (CDrawContext* c, CGraphicsPath* path, const CPoint& centre, double r)
{
	auto g = owned (CGradient::create ({ { 0., hex (0x3a3a3a) }, { 0.6, hex (0x161616) }, { 1., hex (0x050505) } }));
	if (g)
		c->fillRadialGradient (path, *g, CPoint (centre.x - 0.2 * r, centre.y - 0.3 * r), 1.4 * r);
}

void beginDraw (CDrawContext* c)
{
	c->setDrawMode (kAntiAliasing | kNonIntegralMode);
}

} // anonymous namespace

//========================================================================
// PvKnob
//========================================================================
PvKnob::PvKnob (const CPoint& centre, double outer, IControlListener* listener, int32_t tag)
: CControl (CRect (centre.x - outer, centre.y - outer, centre.x + outer, centre.y + outer),
            listener, tag, nullptr)
, mOuter (outer)
{
}

void PvKnob::draw (CDrawContext* context)
{
	beginDraw (context);
	const CRect r = getViewSize ();
	const CPoint c = r.getCenter ();
	const double body = mOuter - (mOuter >= 22. ? 6. : mOuter >= 14. ? 5. : 3.5);
	const int teeth = mOuter >= 14. ? 28 : 18;
	const double dip = mOuter >= 14. ? 1.4 : 0.9;

	// knurled skirt
	if (auto p = owned (context->createGraphicsPath ()))
	{
		for (int i = 0; i < teeth * 2; ++i)
		{
			const CPoint q = pol (c, (i % 2) ? body - dip : body, 360. * i / (teeth * 2));
			if (i == 0) p->beginSubpath (q); else p->addLine (q);
		}
		p->closeSubpath ();
		context->setFillColor (hex (0x0a0a0a));
		context->drawGraphicsPath (p, CDrawContext::kPathFilled);
		context->setLineStyle (kLineSolid);
		context->setLineWidth (0.6);
		context->setFrameColor (hex (0x2f2f2f));
		context->drawGraphicsPath (p, CDrawContext::kPathStroked);
	}
	// cap
	const double cr = body * 0.78;
	if (auto p = owned (context->createGraphicsPath ()))
	{
		p->addEllipse (CRect (c.x - cr, c.y - cr, c.x + cr, c.y + cr));
		capFill (context, p, c, cr);
		context->setLineWidth (0.8);
		context->setFrameColor (hex (0x000000));
		context->drawGraphicsPath (p, CDrawContext::kPathStroked);
	}
	// pointer
	const double ang = -135. + 270. * std::clamp (static_cast<double> (getValueNormalized ()), 0., 1.);
	line (context, pol (c, body * 0.15, ang), pol (c, body - 0.5, ang), Pv::kInk,
	      mOuter >= 22. ? 2.2 : mOuter >= 14. ? 1.8 : 1.4);
	setDirty (false);
}

void PvKnob::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	if (event.clickCount == 2 || event.modifiers.has (ModifierKey::Alt))
	{
		beginEdit ();
		setValueNormalized (getDefaultValue ());
		valueChanged ();
		endEdit ();
		invalid ();
		event.consumed = true;
		return;
	}
	mDragging = true;
	mStartValue = getValueNormalized ();
	mStartPoint = event.mousePosition;
	beginEdit ();
	event.consumed = true;
}

void PvKnob::onMouseMoveEvent (MouseMoveEvent& event)
{
	if (!mDragging)
		return;
	const double range = event.modifiers.has (ModifierKey::Shift) ? kDragRange * 5. : kDragRange;
	const double dy = event.mousePosition.y - mStartPoint.y;
	setValueNormalized (static_cast<float> (std::clamp (mStartValue - dy / range, 0., 1.)));
	valueChanged ();
	invalid ();
	event.consumed = true;
}

void PvKnob::onMouseUpEvent (MouseUpEvent& event)
{
	if (!mDragging)
		return;
	mDragging = false;
	endEdit ();
	event.consumed = true;
}

void PvKnob::onMouseCancelEvent (MouseCancelEvent& event)
{
	if (mDragging) { mDragging = false; endEdit (); }
	event.consumed = true;
}

void PvKnob::onMouseWheelEvent (MouseWheelEvent& event)
{
	const float step = event.modifiers.has (ModifierKey::Shift) ? 0.002f : 0.02f;
	beginEdit ();
	setValueNormalized (std::clamp (getValueNormalized () + static_cast<float> (event.deltaY) * step, 0.f, 1.f));
	valueChanged ();
	endEdit ();
	invalid ();
	event.consumed = true;
}

void PvKnob::onMouseEnterEvent (MouseEnterEvent& event)
{
	if (mHover) mHover->controlHovered (this, true);
	event.consumed = true;
}

void PvKnob::onMouseExitEvent (MouseExitEvent& event)
{
	if (mHover) mHover->controlHovered (this, false);
	event.consumed = true;
}

//========================================================================
// PvSelector
//========================================================================
namespace {
std::vector<double> selectorAngles (size_t n)
{
	if (n == 2) return { -45., 45. };
	if (n == 3) return { -50., 0., 50. };
	return { -75., -25., 25., 75. };
}

struct Legend { CPoint anchor; SciFi::Align align; CRect box; };

Legend legendFor (const CPoint& c, double lr, double t, const std::string& name, double size)
{
	const CPoint p = pol (c, lr, t);
	const bool mid = std::abs (t) < 10.;
	const SciFi::Align a = mid ? SciFi::Align::Centre : (t < 0. ? SciFi::Align::Right : SciFi::Align::Left);
	const double dx = mid ? 0. : (t < 0. ? -1. : 1.);
	const double dy = mid ? -2. : (std::abs (t) > 60. ? 3.5 : 1.);
	const CPoint anchor (p.x + dx, p.y + dy);
	const double w = SciFi::textWidth (name, size, true);
	const double left = a == SciFi::Align::Centre ? anchor.x - w / 2. : a == SciFi::Align::Right ? anchor.x - w : anchor.x;
	const double cap = size * 0.64;
	return { anchor, a, CRect (left - 2., anchor.y - cap - 3., left + w + 2., anchor.y + 3.) };
}
} // anonymous namespace

PvSelector::PvSelector (const CPoint& centre, double radius, std::vector<std::string> names,
                        double legendSize, double legendRadius,
                        IControlListener* listener, int32_t tag)
: CControl (CRect (), listener, tag, nullptr)
, mCentre (centre), mRadius (radius), mLegendSize (legendSize), mLegendRadius (legendRadius)
, mNames (std::move (names))
{
	// The view covers the head and every legend, and no more, so two
	// selectors side by side do not steal each other's clicks.
	CRect box (centre.x - radius - 8., centre.y - radius - 8., centre.x + radius + 8., centre.y + radius + 6.);
	const auto angles = selectorAngles (mNames.size ());
	for (size_t i = 0; i < mNames.size (); ++i)
		box.unite (legendFor (mCentre, mLegendRadius, angles[i], mNames[i], mLegendSize).box);
	setViewSize (box);
	setMouseableArea (box);
	if (mNames.size () > 1)
		setMax (1.f);
}

int PvSelector::step () const
{
	const int n = static_cast<int> (mNames.size ());
	return std::clamp (static_cast<int> (std::lround (getValueNormalized () * (n - 1))), 0, n - 1);
}

void PvSelector::setStep (int s)
{
	const int n = static_cast<int> (mNames.size ());
	s = std::clamp (s, 0, n - 1);
	beginEdit ();
	setValueNormalized (n > 1 ? static_cast<float> (s) / static_cast<float> (n - 1) : 0.f);
	valueChanged ();
	endEdit ();
	invalid ();
}

double PvSelector::angleOf (int i) const
{
	return selectorAngles (mNames.size ())[static_cast<size_t> (i)];
}

void PvSelector::draw (CDrawContext* context)
{
	beginDraw (context);
	const int cur = step ();
	const auto angles = selectorAngles (mNames.size ());
	for (size_t i = 0; i < mNames.size (); ++i)
	{
		const bool active = static_cast<int> (i) == cur;
		const double t = angles[i];
		line (context, pol (mCentre, mRadius + 2.5, t), pol (mCentre, mRadius + 7., t),
		      active ? Pv::kRed : Pv::kTick, active ? 2. : 1.4);
		const Legend lg = legendFor (mCentre, mLegendRadius, t, mNames[i], mLegendSize);
		SciFi::drawText (context, mNames[i], lg.anchor, mLegendSize, active,
		                 active ? Pv::kInk : Pv::kDimLabel, lg.align);
	}

	const double ang = angles[static_cast<size_t> (cur)];
	disc (context, mCentre, mRadius * 0.8, hex (0x0b0b0b));
	ring (context, mCentre, mRadius * 0.8, hex (0x444444), 0.8);
	const CPoint tip = pol (mCentre, mRadius + 3., ang);
	const CPoint l = pol (mCentre, mRadius * 0.5, ang - 90.);
	const CPoint rg = pol (mCentre, mRadius * 0.5, ang + 90.);
	const CPoint tail = pol (mCentre, mRadius, ang + 180.);
	if (auto p = owned (context->createGraphicsPath ()))
	{
		p->beginSubpath (tip);
		p->addLine (rg);
		// quadratic through the tail, as cubic
		const CPoint c1 (rg.x + 2. / 3. * (tail.x - rg.x), rg.y + 2. / 3. * (tail.y - rg.y));
		const CPoint c2 (l.x + 2. / 3. * (tail.x - l.x), l.y + 2. / 3. * (tail.y - l.y));
		p->addBezierCurve (c1, c2, l);
		p->closeSubpath ();
		capFill (context, p, mCentre, mRadius);
		context->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
		context->setLineWidth (0.8);
		context->setFrameColor (hex (0x000000));
		context->drawGraphicsPath (p, CDrawContext::kPathStroked);
	}
	line (context, pol (mCentre, 2., ang), tip, Pv::kInk, 1.6);
	setDirty (false);
}

void PvSelector::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	const auto angles = selectorAngles (mNames.size ());
	for (size_t i = 0; i < mNames.size (); ++i)
		if (legendFor (mCentre, mLegendRadius, angles[i], mNames[i], mLegendSize).box.pointInside (event.mousePosition))
		{
			setStep (static_cast<int> (i));
			event.consumed = true;
			return;
		}
	const int n = static_cast<int> (mNames.size ());
	setStep ((step () + 1) % n);
	event.consumed = true;
}

void PvSelector::onMouseWheelEvent (MouseWheelEvent& event)
{
	if (event.deltaY > 0.) setStep (step () + 1);
	else if (event.deltaY < 0.) setStep (step () - 1);
	event.consumed = true;
}

void PvSelector::onMouseEnterEvent (MouseEnterEvent& event)
{
	if (mHover) mHover->controlHovered (this, true);
	event.consumed = true;
}

void PvSelector::onMouseExitEvent (MouseExitEvent& event)
{
	if (mHover) mHover->controlHovered (this, false);
	event.consumed = true;
}

//========================================================================
// PvLamp
//========================================================================
PvLamp::PvLamp (const CPoint& centre, IControlListener* listener, int32_t tag)
: CControl (CRect (centre.x - 9., centre.y - 9., centre.x + 9., centre.y + 9.), listener, tag, nullptr)
{
}

void PvLamp::draw (CDrawContext* context)
{
	beginDraw (context);
	const CPoint c = getViewSize ().getCenter ();
	const double r = 4.2;
	const bool on = getValueNormalized () >= 0.5f;
	disc (context, c, r + 1.6, hex (0x000000));
	ring (context, c, r + 1.6, hex (0x555555), 0.8);
	if (on)
		disc (context, c, r + 4., Pv::alpha (Pv::kLampOn, 0.22));
	disc (context, c, r, on ? Pv::kLampOn : Pv::kLampOff);
	disc (context, CPoint (c.x - r * 0.3, c.y - r * 0.3), r * 0.32, hex (0xffffff, on ? 0.55 : 0.22));
	setDirty (false);
}

void PvLamp::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	beginEdit ();
	setValueNormalized (getValueNormalized () >= 0.5f ? 0.f : 1.f);
	valueChanged ();
	endEdit ();
	invalid ();
	event.consumed = true;
}

void PvLamp::onMouseEnterEvent (MouseEnterEvent& event)
{
	if (mHover) mHover->controlHovered (this, true);
	event.consumed = true;
}

void PvLamp::onMouseExitEvent (MouseExitEvent& event)
{
	if (mHover) mHover->controlHovered (this, false);
	event.consumed = true;
}

//========================================================================
// PvBadge
//========================================================================
PvBadge::PvBadge (const CPoint& centre, int number, IControlListener* listener, int32_t tag)
: CControl (CRect (centre.x - 11., centre.y - 11., centre.x + 11., centre.y + 11.), listener, tag, nullptr)
, mNumber (number)
{
}

void PvBadge::draw (CDrawContext* context)
{
	beginDraw (context);
	const CPoint c = getViewSize ().getCenter ();
	const bool sel = getValueNormalized () >= 0.5f;
	if (sel)
		disc (context, c, 9.5, Pv::kInk);
	ring (context, c, 9.5, Pv::kInk, 1.3);
	SciFi::drawText (context, std::to_string (mNumber), CPoint (c.x, c.y + 3.8), 10.5, true,
	                 sel ? Pv::kPanel : Pv::kInk, SciFi::Align::Centre);
	setDirty (false);
}

void PvBadge::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	setValueNormalized (1.f);
	valueChanged ();
	invalid ();
	event.consumed = true;
}

//========================================================================
// PvButton
//========================================================================
PvButton::PvButton (const CRect& size, const std::string& text, double textSize,
                    std::function<void ()> onClick)
: CView (size), mText (text), mTextSize (textSize), mOnClick (std::move (onClick))
{
}

void PvButton::setText (const std::string& text)
{
	if (text == mText)
		return;
	mText = text;
	invalid ();
}

void PvButton::draw (CDrawContext* context)
{
	beginDraw (context);
	const CRect r = getViewSize ();
	if (auto p = owned (context->createRoundRectGraphicsPath (r, 2.)))
	{
		context->setFillColor (mDown ? hex (0x2a2a2a) : mHot ? hex (0x1c1c1c) : hex (0x0c0c0c));
		context->drawGraphicsPath (p, CDrawContext::kPathFilled);
		context->setLineStyle (kLineSolid);
		context->setLineWidth (1.1);
		context->setFrameColor (Pv::kStroke);
		context->drawGraphicsPath (p, CDrawContext::kPathStroked);
	}
	SciFi::drawText (context, mText, CPoint (r.getCenter ().x, r.top + r.getHeight () / 2. + 3.4),
	                 mTextSize, true, Pv::kInk, SciFi::Align::Centre);
	setDirty (false);
}

void PvButton::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	mDown = true;
	invalid ();
	event.consumed = true;
}

void PvButton::onMouseUpEvent (MouseUpEvent& event)
{
	if (!mDown)
		return;
	mDown = false;
	invalid ();
	event.consumed = true;
	if (getViewSize ().pointInside (event.mousePosition) && mOnClick)
		mOnClick ();
}

void PvButton::onMouseEnterEvent (MouseEnterEvent& event)
{
	mHot = true;
	invalid ();
	event.consumed = true;
}

void PvButton::onMouseExitEvent (MouseExitEvent& event)
{
	mHot = false;
	invalid ();
	event.consumed = true;
}

//========================================================================
// PvText
//========================================================================
PvText::PvText (const CPoint& anchor, double size, bool bold, SciFi::Align align,
                const CColor& colour, double maxWidth)
: CView (CRect ()), mAnchor (anchor), mSize (size), mBold (bold), mAlign (align), mColour (colour)
{
	const double cap = size * 0.64;
	double l = anchor.x - 2., r = anchor.x + maxWidth;
	if (align == SciFi::Align::Centre) { l = anchor.x - maxWidth / 2.; r = anchor.x + maxWidth / 2.; }
	else if (align == SciFi::Align::Right) { l = anchor.x - maxWidth; r = anchor.x + 2.; }
	setViewSize (CRect (l, anchor.y - cap - 3., r, anchor.y + 3.));
	setMouseEnabled (false);
}

void PvText::setText (const std::string& text)
{
	if (text == mText)
		return;
	mText = text;
	invalid ();
}

void PvText::setColour (const CColor& colour)
{
	if (colour == mColour)
		return;
	mColour = colour;
	invalid ();
}

void PvText::draw (CDrawContext* context)
{
	SciFi::drawText (context, mText, mAnchor, mSize, mBold, mColour, mAlign);
	setDirty (false);
}

//========================================================================
// PvOled
//========================================================================
PvOled::PvOled (const CRect& size)
: CView (size)
{
	setMouseEnabled (false);
}

void PvOled::draw (CDrawContext* context)
{
	beginDraw (context);
	const CRect r = getViewSize ();
	context->saveGlobalState ();
	{
		ConcatClip clip (*context, r);
		drawContent (context);

		// the glass: a faint sheen across the top, and a hairline highlight
		if (auto p = owned (context->createGraphicsPath ()))
		{
			p->addRect (r);
			auto g = owned (CGradient::create ({ { 0., hex (0xffffff, 0.07) }, { 0.45, hex (0xffffff, 0.015) },
			                                     { 0.46, hex (0xffffff, 0.) }, { 1., hex (0xffffff, 0.) } }));
			if (g)
				context->fillLinearGradient (p, *g, r.getTopLeft (),
				                             CPoint (r.left + 0.35 * r.getWidth (), r.bottom));
		}
		line (context, CPoint (r.left + 3., r.top + 0.8), CPoint (r.right - 3., r.top + 0.8),
		      hex (0xffffff, 0.12), 0.6);
	}
	context->restoreGlobalState ();
	setDirty (false);
}

void PvOled::trace (CDrawContext* context, const std::vector<CPoint>& pts, const CColor& colour, double width)
{
	if (pts.size () < 2)
		return;
	auto p = owned (context->createGraphicsPath ());
	if (!p)
		return;
	p->beginSubpath (pts[0]);
	for (size_t i = 1; i < pts.size (); ++i)
		p->addLine (pts[i]);
	context->setLineStyle (CLineStyle (CLineStyle::kLineCapRound, CLineStyle::kLineJoinRound));
	context->setLineWidth (width * 2.6);
	context->setFrameColor (Pv::alpha (colour, 0.22));
	context->drawGraphicsPath (p, CDrawContext::kPathStroked);
	context->setLineWidth (width);
	context->setFrameColor (colour);
	context->drawGraphicsPath (p, CDrawContext::kPathStroked);
}

void PvOled::fillUnder (CDrawContext* context, const std::vector<CPoint>& pts, double base, double top,
                        const CColor& colour)
{
	if (pts.size () < 2)
		return;
	auto p = owned (context->createGraphicsPath ());
	if (!p)
		return;
	p->beginSubpath (pts[0]);
	for (size_t i = 1; i < pts.size (); ++i)
		p->addLine (pts[i]);
	p->addLine (CPoint (pts.back ().x, base));
	p->addLine (CPoint (pts.front ().x, base));
	p->closeSubpath ();
	auto g = owned (CGradient::create (0., 1., Pv::alpha (colour, 0.38), Pv::alpha (colour, 0.02)));
	if (g)
		context->fillLinearGradient (p, *g, CPoint (0., top), CPoint (0., base));
}

void PvOled::oledText (CDrawContext* context, const std::string& text, double x, double y,
                       const CColor& colour, SciFi::Align align, double size)
{
	SciFi::drawText (context, text, CPoint (x, y), size, false, colour, align);
}

//========================================================================
// PvBarsOled
//========================================================================
void PvBarsOled::setLevels (const std::array<float, 10>& levels, int selected)
{
	if (levels == mLevels && selected == mSelected)
		return;
	mLevels = levels;
	mSelected = selected;
	invalid ();
}

void PvBarsOled::setLive (bool live, const std::array<float, 10>& amplitude)
{
	if (live == mLive && (!live || amplitude == mLiveAmp))
		return;
	mLive = live;
	mLiveAmp = amplitude;
	invalid ();
}

void PvBarsOled::setWave (const float* samples, int count)
{
	if (!samples || count < 2)
		mWave.clear ();
	else
		mWave.assign (samples, samples + count);
	invalid ();
}

void PvBarsOled::drawContent (CDrawContext* context)
{
	const CRect r = getViewSize ();
	const double base = r.top + 94.;     // 200 on a screen at y 106
	const double segH = 5.;

	// the output, faint, behind the bars
	if (mLive && mWave.size () > 1)
	{
		float peak = 1e-6f;
		for (float v : mWave)
			peak = std::max (peak, std::abs (v));
		const double mid = (r.top + 20. + base) / 2., amp = (base - r.top - 24.) / 2.;
		std::vector<CPoint> pts;
		const size_t n = mWave.size ();
		for (size_t i = 0; i < n; ++i)
			pts.emplace_back (r.left + 6. + (r.getWidth () - 12.) * static_cast<double> (i) / static_cast<double> (n - 1),
			                  mid - amp * mWave[i] / peak);
		trace (context, pts, hex (0xeef3ff, 0.22), 1.);
	}

	for (int i = 0; i < 10; ++i)
	{
		const double x = r.left + 48. + i * 44.8;
		const bool sel = i == mSelected;
		const float shown = mLive ? mLiveAmp[static_cast<size_t> (i)] : mLevels[static_cast<size_t> (i)];
		const int segs = static_cast<int> (std::lround (std::clamp (shown, 0.f, 1.f) * 14.));
		context->setFillColor (sel ? Pv::kAmber : Pv::alpha (Pv::kAmber, 0.5));
		for (int s = 0; s < segs; ++s)
			context->drawRect (CRect (x - 13., base - 5. - s * segH, x + 13., base - 5. - s * segH + 3.4), kDrawFilled);
		if (mLive)
		{
			// where the partial is set to, as a tick over its live bar
			const int set = static_cast<int> (std::lround (std::clamp (mLevels[static_cast<size_t> (i)], 0.f, 1.f) * 14.));
			if (set > 0)
			{
				const double y = base - 5. - (set - 1) * segH;
				context->setFillColor (Pv::alpha (Pv::kAmber, 0.35));
				context->drawRect (CRect (x - 13., y, x + 13., y + 1.2), kDrawFilled);
			}
		}
		oledText (context, std::to_string (i + 1), x, r.top + 104., sel ? Pv::kAmber : Pv::kOledText,
		          SciFi::Align::Centre, 8.);
	}
	const int pct = static_cast<int> (std::lround (mLevels[static_cast<size_t> (mSelected)] * 100.));
	oledText (context, "P" + std::to_string (mSelected + 1) + "  " + std::to_string (pct) + " %",
	          r.right - 8., r.top + 13., Pv::kAmber, SciFi::Align::Right);
}

//========================================================================
// PvFilterOled
//========================================================================
void PvFilterOled::setResponse (std::vector<float> freq, std::vector<float> mag, const std::string& annotation)
{
	mFreq = std::move (freq);
	mMag = std::move (mag);
	mAnnotation = annotation;
	invalid ();
}

void PvFilterOled::setPartials (const std::array<float, 10>& hz, const std::array<float, 10>& mag,
                                int selected, double sweepHz, const std::string& caption)
{
	if (hz == mPartialHz && mag == mPartialMag && selected == mSelected && sweepHz == mSweepHz &&
	    caption == mCaption)
		return;
	mPartialHz = hz;
	mPartialMag = mag;
	mSelected = selected;
	mSweepHz = sweepHz;
	mCaption = caption;
	invalid ();
}

double PvFilterOled::xOf (double hz) const
{
	const CRect r = getViewSize ();
	return r.left + 6. + std::log (std::max (hz, 1.) / 20.) / std::log (1000.) * (r.getWidth () - 12.);
}

double PvFilterOled::yOf (double m) const
{
	const CRect r = getViewSize ();
	return r.bottom - 18. - (std::clamp (m, 0., 1.25) / 1.3) * (r.getHeight () - 42.);
}

void PvFilterOled::drawContent (CDrawContext* context)
{
	const CRect r = getViewSize ();
	const double base = r.bottom - 18.;
	std::vector<CPoint> pts;
	for (size_t i = 0; i < mFreq.size () && i < mMag.size (); ++i)
		if (mFreq[i] >= 20.f && mFreq[i] <= 20000.f)
			pts.emplace_back (xOf (mFreq[i]), yOf (mMag[i]));
	fillUnder (context, pts, base, yOf (1.25), Pv::kBlue);
	trace (context, pts, Pv::kBlue);

	// the sweep: where the fundamental is being read
	if (mSweepHz > 0.)
	{
		const double x = xOf (mSweepHz);
		context->setFillColor (Pv::alpha (Pv::kAmber, 0.10));
		context->drawRect (CRect (x - 3., r.top + 18., x + 3., base), kDrawFilled);
		line (context, CPoint (x, r.top + 18.), CPoint (x, base), Pv::alpha (Pv::kAmber, 0.6), 1.);
	}

	for (int k = 0; k < 10; ++k)
	{
		const double hz = mPartialHz[static_cast<size_t> (k)];
		if (hz < 20. || hz > 20000.)
			continue;
		const CPoint p (xOf (hz), yOf (mPartialMag[static_cast<size_t> (k)]));
		line (context, p, CPoint (p.x, base), Pv::alpha (Pv::kAmber, 0.4), 1.);
		if (k == mSelected)
		{
			disc (context, p, 4., hex (0x000000));
			ring (context, p, 4., hex (0xffffff), 1.6);
		}
		else
			disc (context, p, 2.6, Pv::kAmber);
	}
	if (!mCaption.empty ())
		oledText (context, mCaption, r.left + 8., r.top + 13., Pv::kOledText);
	oledText (context, mAnnotation, r.right - 8., r.top + 13., Pv::kBlue, SciFi::Align::Right);
}

//========================================================================
// PvEnvOled
//========================================================================
PvEnvOled::PvEnvOled (const CRect& size, const CColor& colour, bool labelled)
: PvOled (size), mColour (colour), mLabelled (labelled)
{
}

void PvEnvOled::setEnvelope (std::vector<float> shape, double keyUp,
                             const std::string& caption, const std::string& annotation)
{
	mShape = std::move (shape);
	mKeyUp = keyUp;
	mCaption = caption;
	mAnnotation = annotation;
	invalid ();
}

void PvEnvOled::setPlayhead (double position, double value)
{
	if (position < 0. && mPlayPos < 0.)
		return;
	mPlayPos = position;
	mPlayValue = value;
	invalid ();
}

void PvEnvOled::drawContent (CDrawContext* context)
{
	const CRect r = getViewSize ();
	const double top = mLabelled ? 20. : 8.;
	const double x0 = r.left + 6., x1 = r.right - 6., base = r.bottom - 6.;
	const double span = r.getHeight () - top - 6.;
	std::vector<CPoint> pts;
	const size_t n = mShape.size ();
	for (size_t i = 0; i < n; ++i)
	{
		const double t = n > 1 ? static_cast<double> (i) / static_cast<double> (n - 1) : 0.;
		pts.emplace_back (x0 + t * (x1 - x0), base - std::clamp (static_cast<double> (mShape[i]), 0., 1.) * span);
	}
	fillUnder (context, pts, base, base - span, mColour);
	if (mKeyUp >= 0. && mKeyUp <= 1.)
	{
		const double kx = x0 + mKeyUp * (x1 - x0);
		const CCoord dash[] = { 3., 3. };
		context->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter, 0., 2, dash));
		context->setLineWidth (1.);
		context->setFrameColor (Pv::kOledText);
		context->drawLine (CPoint (kx, r.top + (mLabelled ? 18. : 4.)), CPoint (kx, r.bottom - 4.));
	}
	trace (context, pts, mColour);
	if (mPlayPos >= 0.)
	{
		const double x = x0 + std::min (mPlayPos, 1.) * (x1 - x0);
		const double y = base - std::clamp (mPlayValue, 0., 1.) * span;
		line (context, CPoint (x, r.top + (mLabelled ? 18. : 4.)), CPoint (x, base), Pv::alpha (mColour, 0.35), 1.);
		disc (context, CPoint (x, y), 6., Pv::alpha (mColour, 0.25));
		disc (context, CPoint (x, y), 3., hex (0xffffff));
		ring (context, CPoint (x, y), 3., mColour, 1.2);
	}
	if (!mCaption.empty ())
		oledText (context, mCaption, r.left + 8., r.top + 13., Pv::kOledText);
	if (!mAnnotation.empty ())
		oledText (context, mAnnotation, r.right - 8., r.top + 13., mColour, SciFi::Align::Right);
}

//========================================================================
// PvRingsOled
//========================================================================
PvRingsOled::PvRingsOled (const CRect& size, std::function<void (int)> onPick)
: PvOled (size), mOnPick (std::move (onPick))
{
	setMouseEnabled (true);
}

void PvRingsOled::setLfos (const std::array<float, 10>& depth, const std::array<std::string, 10>& rate,
                           int selected)
{
	if (depth == mDepth && rate == mRate && selected == mSelected)
		return;
	mDepth = depth;
	mRate = rate;
	mSelected = selected;
	invalid ();
}

void PvRingsOled::setPhases (bool live, const std::array<float, 10>& phase)
{
	if (live == mLive && (!live || phase == mPhase))
		return;
	mLive = live;
	mPhase = phase;
	invalid ();
}

void PvRingsOled::drawContent (CDrawContext* context)
{
	const CRect r = getViewSize ();
	for (int i = 0; i < 10; ++i)
	{
		const CPoint c (r.left + 16. + i * 29.5, r.top + 60.);
		const double rr = 11.;
		const bool sel = i == mSelected;
		const float dv = mDepth[static_cast<size_t> (i)];
		ring (context, c, rr, hex (0x1c1c26), 2.4);
		if (dv > 0.f)
		{
			const double a = -135. + 270. * std::min (1.f, dv);
			if (auto p = owned (context->createGraphicsPath ()))
			{
				// VSTGUI arcs count from 3 o'clock; the mock-up's from 12.
				p->addArc (CRect (c.x - rr, c.y - rr, c.x + rr, c.y + rr), -225., a - 90., true);
				context->setLineStyle (CLineStyle (CLineStyle::kLineCapButt, CLineStyle::kLineJoinMiter));
				context->setLineWidth (5.);
				context->setFrameColor (Pv::alpha (Pv::kViolet, 0.2));
				context->drawGraphicsPath (p, CDrawContext::kPathStroked);
				context->setLineWidth (2.4);
				context->setFrameColor (sel ? Pv::kViolet : Pv::alpha (Pv::kViolet, 0.6));
				context->drawGraphicsPath (p, CDrawContext::kPathStroked);
			}
			oledText (context, mRate[static_cast<size_t> (i)], c.x, c.y + 4., Pv::kViolet, SciFi::Align::Centre, 7.);
			if (mLive)
			{
				// the LFO itself, running round the ring
				const double deg = mPhase[static_cast<size_t> (i)] * 180. / kPi;
				const CPoint d = pol (c, rr, deg);
				disc (context, d, 4., Pv::alpha (Pv::kViolet, 0.3));
				disc (context, d, 2.2, hex (0xffffff));
			}
		}
		oledText (context, std::to_string (i + 1), c.x, r.top + 96., sel ? Pv::kViolet : Pv::kOledText,
		          SciFi::Align::Centre, 8.);
	}
}

void PvRingsOled::onMouseDownEvent (MouseDownEvent& event)
{
	if (!event.buttonState.isLeft ())
		return;
	const CRect r = getViewSize ();
	const int i = static_cast<int> (std::lround ((event.mousePosition.x - (r.left + 16.)) / 29.5));
	if (i >= 0 && i < 10 && mOnPick)
		mOnPick (i);
	event.consumed = true;
}

//========================================================================
// PvVoiceMeter
//========================================================================
void PvVoiceMeter::setVoices (int count)
{
	if (count == mVoices)
		return;
	mVoices = count;
	invalid ();
}

void PvVoiceMeter::drawContent (CDrawContext* context)
{
	const CRect r = getViewSize ();
	for (int i = 0; i < 16; ++i)
	{
		const CRect seg (r.left + 6. + i * 12.5, r.top + 5., r.left + 14. + i * 12.5, r.top + 13.);
		const bool on = i < mVoices;
		if (on)
		{
			context->setFillColor (Pv::alpha (Pv::kMint, 0.25));
			context->drawRect (CRect (seg).extend (2., 2.), kDrawFilled);
		}
		context->setFillColor (on ? Pv::kMint : hex (0x14211b));
		context->drawRect (seg, kDrawFilled);
	}
}

//========================================================================
// PvReadout
//========================================================================
PvReadout::PvReadout (const CRect& size)
: CView (size)
{
	setMouseEnabled (false);
}

void PvReadout::setText (const std::string& text)
{
	if (text == mText)
		return;
	mText = text;
	invalid ();
}

void PvReadout::draw (CDrawContext* context)
{
	if (mText.empty ())
		return;
	beginDraw (context);
	const CRect v = getViewSize ();
	const double w = SciFi::textWidth (mText, 9., true) + 16.;
	double cx = mAnchorX >= 0. ? mAnchorX : v.getCenter ().x;
	cx = std::clamp (cx, v.left + w / 2. + 1., v.right - w / 2. - 1.);
	const CRect plate (cx - w / 2., v.top, cx + w / 2., v.bottom);
	context->setFillColor (hex (0x000000));
	context->drawRect (plate, kDrawFilled);
	context->setLineStyle (kLineSolid);
	context->setLineWidth (1.);
	context->setFrameColor (Pv::kStroke);
	context->drawRect (plate, kDrawStroked);
	SciFi::drawText (context, mText, CPoint (cx, v.top + v.getHeight () / 2. + 3.), 9., true,
	                 Pv::kInk, SciFi::Align::Centre);
	setDirty (false);
}

//========================================================================
// PvOverlay
//========================================================================
PvOverlay::PvOverlay (const CRect& size, CBitmap* bitmap)
: CView (size), mBitmap (bitmap)
{
	setMouseEnabled (false);
}

void PvOverlay::draw (CDrawContext* context)
{
	if (mBitmap)
		context->drawBitmap (mBitmap, getViewSize ());
	setDirty (false);
}

//------------------------------------------------------------------------
} // namespace ForTran
