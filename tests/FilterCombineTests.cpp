//------------------------------------------------------------------------
// ForTran - filter shape combination checks
//
//   c++ -std=c++17 -O2 -Wall -Wextra -I../source ../source/FilterInfo.cpp FilterCombineTests.cpp -o combine-tests
//
// `originalCombine` below reproduces synthNote's 600 lines of hand-unrolled
// switch exactly as written, bugs included. The port is expected to AGREE
// with it everywhere the original was not broken, and to DIFFER in precisely
// the three ways PORTING-NOTES section 6 records. Both halves are asserted:
// an agreement test alone would not catch a fix that went too far.
//------------------------------------------------------------------------

#include "FilterInfo.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <initializer_list>

using namespace ForTran;
static int fails = 0;
static void chk (bool ok, const char* what)
{ if (!ok) { std::printf("  FAIL: %s\n", what); ++fails; } }

//------------------------------------------------------------------------
// synthNote, lines 894-1497, transcribed.
static void originalCombine (const FilterInfo& fi, int lp, int hp, int cb, int gr,
                             int bin, float* M, float* T)
{
	if (lp == 1)
		fi.values (FilterInfo::Shape::LowPass, bin, M, T);
	else
		for (int i = 0; i < 10; ++i) { T[i] = 0.f; M[i] = 1.f; }

	float Sm[10], St[10];

	// --- high pass
	if (hp == 1) {
		fi.values (FilterInfo::Shape::HighPass, bin, Sm, St);
		for (int i = 0; i < 10; ++i) T[i] = T[i] + St[i];
		if (lp != 0) for (int i = 0; i < 10; ++i) M[i] = M[i] + Sm[i];
		else         for (int i = 0; i < 10; ++i) M[i] = Sm[i];
	} else if (hp == 2) {
		fi.values (FilterInfo::Shape::HighPass, bin, Sm, St);
		for (int i = 0; i < 10; ++i) { T[i] = T[i] * St[i]; M[i] = M[i] * Sm[i]; }
	} else if (hp == 3) {
		fi.values (FilterInfo::Shape::HighPass, bin, Sm, St);
		for (int i = 0; i < 10; ++i) St[i] = std::max (T[i], St[i]);   // into scratch: discarded
		if (lp != 0) { for (int i = 0; i < 10; ++i) Sm[i] = std::max (M[i], Sm[i]); } // discarded
		else         { for (int i = 0; i < 10; ++i) M[i] = Sm[i]; }
	}

	// --- comb
	if (cb == 1) {
		fi.values (FilterInfo::Shape::Comb, bin, Sm, St);
		for (int i = 0; i < 10; ++i) T[i] = T[i] + St[i];
		if (lp != 0)      for (int i = 0; i < 10; ++i) M[i] = M[i] + Sm[i];
		else if (hp != 0) for (int i = 0; i < 10; ++i) M[i] = M[i] + Sm[i];
		else              for (int i = 0; i < 10; ++i) M[i] = Sm[i];
	} else if (cb == 2) {
		fi.values (FilterInfo::Shape::Comb, bin, Sm, St);
		for (int i = 0; i < 10; ++i) { T[i] = T[i] * St[i]; M[i] = M[i] * Sm[i]; }
	} else if (cb == 3) {
		fi.values (FilterInfo::Shape::Comb, bin, Sm, St);
		for (int i = 0; i < 10; ++i) St[i] = std::max (T[i], St[i]);
		if (lp != 0) { for (int i = 0; i < 10; ++i) Sm[i] = std::max (M[i], Sm[i]); }
		else         { for (int i = 0; i < 10; ++i) M[i] = Sm[i]; }
	}

	// --- grass  (note: the else-if tests hp, never cb)
	if (gr == 1) {
		fi.values (FilterInfo::Shape::Grass, bin, Sm, St);
		for (int i = 0; i < 10; ++i) T[i] = T[i] + St[i];
		if (lp != 0)      for (int i = 0; i < 10; ++i) M[i] = M[i] + Sm[i];
		else if (hp != 0) for (int i = 0; i < 10; ++i) M[i] = M[i] + Sm[i];
		else              for (int i = 0; i < 10; ++i) M[i] = Sm[i];
	} else if (gr == 2) {
		fi.values (FilterInfo::Shape::Grass, bin, Sm, St);
		for (int i = 0; i < 10; ++i) { T[i] = T[i] * St[i]; M[i] = M[i] * Sm[i]; }
	} else if (gr == 3) {
		fi.values (FilterInfo::Shape::Grass, bin, Sm, St);
		for (int i = 0; i < 10; ++i) St[i] = std::max (T[i], St[i]);
		if (lp != 0) { for (int i = 0; i < 10; ++i) Sm[i] = std::max (M[i], Sm[i]); }
		else         { for (int i = 0; i < 10; ++i) M[i] = Sm[i]; }
	}
}

static void portCombine (const FilterInfo& fi, int lp, int hp, int cb, int gr,
                         int bin, float* M, float* T)
{
	FilterInfo::ShapeStates st; st.lowPass = lp; st.highPass = hp; st.comb = cb; st.grass = gr;
	fi.combine (st, bin, M, T);
}

static bool sameMag (const float* a, const float* b)
{ for (int i = 0; i < 10; ++i) if (a[i] != b[i]) return false; return true; }

//------------------------------------------------------------------------
int main ()
{
	FilterInfo fi;
	const int kBin = 200;

	// 1. Everywhere the original was not broken, the port must match it.
	//    "Not broken" = no Max stage, and no combination where a later Add
	//    discards an earlier shape.
	std::printf("1. agreement with the original where the original worked\n");
	int agreed = 0, differed = 0;
	for (int lp = 0; lp <= 1; ++lp)
	 for (int hp = 0; hp <= 3; ++hp)
	  for (int cb = 0; cb <= 3; ++cb)
	   for (int gr = 0; gr <= 3; ++gr)
	   {
		   const bool anyMax  = (hp == 3 || cb == 3 || gr == 3);
		   // the comb-discarding case: LP and HP both off, comb contributed,
		   // then grass assigns over it
		   const bool clobber = (lp == 0 && hp == 0 && cb != 0 && gr != 0);
		   // Mult on phase: the original multiplies, the port passes through
		   const bool multPhase = (hp == 2 || cb == 2 || gr == 2);
		   if (anyMax || clobber) { ++differed; continue; }

		   float oM[10], oT[10], pM[10], pT[10];
		   originalCombine (fi, lp, hp, cb, gr, kBin, oM, oT);
		   portCombine     (fi, lp, hp, cb, gr, kBin, pM, pT);

		   chk (sameMag (oM, pM), "magnitude differs where the original was sound");
		   if (!multPhase)
			   for (int i = 0; i < 10; ++i)
				   chk (oT[i] == pT[i], "phase differs where the original was sound");
		   ++agreed;
	   }
	std::printf("   %d of %d state combinations compared, %d excluded as known-broken\n",
	            agreed, 2*4*4*4, differed);
	chk (agreed > 40, "too few combinations were actually compared");

	// 2. Max now does something when low pass is on. In the original it did
	//    nothing whatsoever.
	std::printf("2. Max, with low pass on\n");
	{
		float oM[10], oT[10], pM[10], pT[10], lpM[10], lpT[10];
		originalCombine (fi, 1, 3, 0, 0, kBin, oM, oT);
		portCombine     (fi, 1, 3, 0, 0, kBin, pM, pT);
		portCombine     (fi, 1, 0, 0, 0, kBin, lpM, lpT);
		chk (sameMag (oM, lpM), "expected the original's Max to be a no-op - check the transcription");
		chk (!sameMag (pM, lpM), "the port's Max still does nothing");
		float hpM[10], hpT[10];
		fi.values (FilterInfo::Shape::HighPass, kBin, hpM, hpT);
		bool isMax = true;
		for (int i = 0; i < 10; ++i)
			if (pM[i] != std::max (lpM[i], hpM[i])) isMax = false;
		chk (isMax, "the port's Max is not max(running, shape)");
		std::printf("   original == low pass alone: %s;  port == max(lp, hp): %s\n",
		            sameMag (oM, lpM) ? "yes" : "no", isMax ? "yes" : "no");
	}

	// 3. The comb contribution is no longer discarded by a following grass.
	std::printf("3. LP off, HP off, Comb=Add, Grass=Add\n");
	{
		float oM[10], oT[10], pM[10], pT[10], cbM[10], cbT[10], grM[10], grT[10];
		originalCombine (fi, 0, 0, 1, 1, kBin, oM, oT);
		portCombine     (fi, 0, 0, 1, 1, kBin, pM, pT);
		fi.values (FilterInfo::Shape::Comb,  kBin, cbM, cbT);
		fi.values (FilterInfo::Shape::Grass, kBin, grM, grT);
		bool origIsGrassOnly = sameMag (oM, grM);
		bool portIsSum = true;
		for (int i = 0; i < 10; ++i) if (pM[i] != cbM[i] + grM[i]) portIsSum = false;
		std::printf("   original == grass alone (comb discarded): %s\n", origIsGrassOnly ? "yes" : "no");
		std::printf("   port     == comb + grass:                 %s\n", portIsSum ? "yes" : "no");
		chk (origIsGrassOnly, "expected the original to discard the comb - check the transcription");
		chk (portIsSum, "the port does not sum comb and grass");
	}

	// 4. Mult and the phase.
	//
	//    The divergence is narrower than it first looks. With low pass OFF
	//    the phase accumulator starts at zero, so the original's multiply
	//    gives zero - but so does leaving it alone, and a following Add
	//    revives it either way. The two only part company when there is a
	//    phase to multiply, i.e. when low pass is on.
	std::printf("4. Mult and the phase\n");
	{
		float oM[10], oT[10], pM[10], pT[10], lpM[10], lpT[10];

		// low pass off: no difference at all, before or after a following Add
		originalCombine (fi, 0, 2, 1, 0, kBin, oM, oT);
		portCombine     (fi, 0, 2, 1, 0, kBin, pM, pT);
		bool sameWhenLpOff = true;
		for (int i = 0; i < 10; ++i) if (oT[i] != pT[i]) sameWhenLpOff = false;
		std::printf("   LP off, HP=Mult, Comb=Add - phase identical: %s\n",
		            sameWhenLpOff ? "yes" : "no");
		chk (sameWhenLpOff, "phase should not differ when low pass is off");

		// low pass on: the original multiplies radians by radians
		originalCombine (fi, 1, 2, 0, 0, kBin, oM, oT);
		portCombine     (fi, 1, 2, 0, 0, kBin, pM, pT);
		fi.values (FilterInfo::Shape::LowPass, kBin, lpM, lpT);
		float hpM[10], hpT[10];
		fi.values (FilterInfo::Shape::HighPass, kBin, hpM, hpT);
		bool origIsProduct = true, portIsPassthrough = true;
		for (int i = 0; i < 10; ++i)
		{
			if (oT[i] != lpT[i] * hpT[i]) origIsProduct = false;
			if (pT[i] != lpT[i])          portIsPassthrough = false;
		}
		std::printf("   LP on,  HP=Mult - original phase = lp*hp: %s;  port passes lp through: %s\n",
		            origIsProduct ? "yes" : "no", portIsPassthrough ? "yes" : "no");
		chk (origIsProduct, "expected the original to multiply the phases");
		chk (portIsPassthrough, "the port does not pass the phase through Mult");
		chk (sameMag (oM, pM), "Mult magnitude should be unchanged from the original");
	}

	// 5. All off is the identity, and nothing is ever left non-finite.
	std::printf("5. identity and finiteness over every state and bin\n");
	{
		float M[10], T[10];
		portCombine (fi, 0, 0, 0, 0, kBin, M, T);
		bool identity = true;
		for (int i = 0; i < 10; ++i) if (M[i] != 1.f || T[i] != 0.f) identity = false;
		chk (identity, "all-off is not the identity");

		int bad = 0;
		for (int lp = 0; lp <= 1; ++lp)
		 for (int hp = 0; hp <= 3; ++hp)
		  for (int cb = 0; cb <= 3; ++cb)
		   for (int gr = 0; gr <= 3; ++gr)
			for (int bin : {0, 1, 137, 276, 400, 551})
			{
				portCombine (fi, lp, hp, cb, gr, bin, M, T);
				for (int i = 0; i < 10; ++i)
					if (!std::isfinite (M[i]) || !std::isfinite (T[i]) || M[i] < 0.f) ++bad;
			}
		std::printf("   %d bad values across every state combination x 6 bins\n", bad);
		chk (bad == 0, "combine produced a bad value");
	}

	std::printf(fails ? "\n%d CHECK(S) FAILED\n" : "\nall checks passed\n", fails);
	return fails != 0;
}
