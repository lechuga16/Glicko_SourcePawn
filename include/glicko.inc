/**
 * =============================================================================
 * Glicko SourcePawn (C)2022-2022 Lechuga.  All rights reserved.
 * =============================================================================
 *
 * This file is part of the Glicko SourcePawn.
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 3.0, as published by the
 * Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Sources and documentation:
 * https://rhetoricstudios.com/downloads/AbstractingGlicko2ForTeamGames.pdf
 * http://www.glicko.net/glicko/glicko.pdf
 * https://eprints.ucm.es/id/eprint/66998/1/ORY_ALONSO_Sistema_de_matchmaking_para_un_videojuego_multijugador_784051_194709458.pdf
 *
 */

#if defined _glicko_included
	#endinput
#endif
#define _glicko_included

#include <float>

/* Activate this variable if you want to create modified PlayerInfo.
 * This is useful if you want to add more variables to the PlayerInfo struct.
 * You can also use this to create a custom PlayerInfo struct.
 *
 * #define CUSTOM_PLAYERINFO
 */

#define DEFAULT_RATING	  1500.0	// DEFAULT 1500.0
#define DEFAULT_DEVIATION 350.0		// DEFAULT 350.0

#define E_NUMBER		  2.71828182845904523536028747135266249775724709369995	  // Euler's number
#define INITIAL_DEVIATION 350.0													  // DEFAULT 350.0
#define MEAN_DEVIATION	  50.0													  // DEFAULT 50.0
#define POINT_PERIODS	  100.0													  // DEFAULT 100.0

/**
 * Define the possible outcomes.
 */
enum MatchResults
{
	Result_Loss = 0,
	Result_Draw = 1,
	Result_Win	= 2,

	Size_Result = 3
};

/**
 * Defines a value the possible results..
 */
stock float	 g_fResult[Size_Result] = { 0.0, 0.5, 1.0 };

#if !defined CUSTOM_PLAYERINFO
/**
 * Player profile.
 */
enum struct PlayerInfo
{
	float rating;		// Glicko Score
	float deviation;	// Glicko deviation
}
#endif

/**
 * Defines the value of the constant c.
 *
 * @return              value of c.
 * @note                c = √( (350^2 − 50^2) / 100) ≈ 34.6
 */
stock float Glicko_c()
{
	return SquareRoot((Square(INITIAL_DEVIATION) - Square(MEAN_DEVIATION)) / POINT_PERIODS);
}

/**
 * Defines the previous deviation of grades. If the Player is in the qualifying period, this value should be calculated as INITIAL_DEVIATION.
 *
 * @param Player       Player profile.
 * @return             value of RD.
 * @note               RD = √( RD^2 − c^2 )
 */
stock float Glicko_RD(PlayerInfo Player)
{
	return SquareRoot(Square(Player.deviation) + Square(Glicko_c()));
}

/**
 * Defines the value of the constant q.
 *
 * @return             value of q.
 * @note               q = ln(10) / 400 ≈ 0.0057565
 */
stock float Glicko_q()
{
	return Logarithm(10.0, E_NUMBER) / 400.0;
}

/**
 * Defines the value of g(RD).
 *
 * @param Player       Player profile.
 * @return             value of g(RD).
 * @note               g(RD) = 1 / √( 1 + (3 * q^2 * RD^2) / π^2 )
 */
stock float Glicko_g(PlayerInfo Player)
{
	return 1.0 / SquareRoot(1.0 + (3.0 * Square(Glicko_q()) * Square(Glicko_RD(Player))) / Square(FLOAT_PI));
}

/**
 * Defines the value of E.
 *
 * @param Player       Player profile.
 * @param Opponent     Opponent profile.
 * @return             value of E.
 * @note               E(s|r, rj, RDj ) = 1 / (1 + 10^((g(RD) * (r - rj)) / 400))
 */
stock float Glicko_e(PlayerInfo Player, PlayerInfo Opponent)
{
	return 1.0 / (1.0 + Pow(10.0, (Glicko_g(Player) * (Player.rating - Opponent.rating)) / 400.0));
}

/**
 * Defines the value of d^2. The sum responds to the number of Opponents in the scoring period.
 *
 * @param Player       Player profile.
 * @param Opponent     Opponent profile.
 * @return             value of Σ.
 * @note               Σ( g(RD)^2 * E(s|r,rj,RDj) * (1 - E(s|r,rj,RDj)) )
 * @note               Σ = Glicko_sum_d
 */
stock float Glicko_sum_d(PlayerInfo Player, PlayerInfo Opponent)
{
	float
		f_g = Glicko_g(Opponent),			 // g(RD)
		f_e = Glicko_e(Opponent, Player);	 // E(s|r,rj,RDj)

	return Square(f_g) * f_e * (1 - f_e);
}

/**
 * Defines the value of d^2. The sum responds to the number of Opponents in the scoring period.
 *
 * @param fSum_d       value of Σ.
 * @return             value of d (d_Full).
 * @note               d = √( 1 / (q^2 * Σ( g(RD)^2 * E(s|r,rj,RDj) * (1 - E(s|r,rj,RDj)) ) ) )
 */
stock float Glicko_d(float fSum_d)
{
	return SquareRoot(1.0 / (Square(Glicko_q()) * fSum_d));
}

/**
 * Defines the value of RD'.
 *
 * @param Player       Player profile.
 * @param d_Full       value of d.
 * @return             value of RD'.
 * @note               RD' = √( 1 / (1 / RD^2 + 1 / d^2) )
 */
stock float Glicko_FinalRD(int client, PlayerInfo Player, float d_Full)
{
	float
		fSquare_RD	   = 1.0 / Square(Player.deviation),	// 1 / RD^2
		fSquare_d_Full = 1.0 / Square(d_Full);				// 1 / d^2

	return SquareRoot(1.0 / (fSquare_RD + fSquare_d_Full));
}

/**
 * Defines the value of Σ in Final Rating.
 *
 * @param Player       Player profile.
 * @param Opponent     Opponent profile.
 * @param Result       Result of the match.
 * @return             value of Σ.
 * @note               Σ( g(RD) * (s - E(s|r,rj,RDj)) )
 * @note               Σ = Glicko_sum_FinalRating
 */
stock float Glicko_sum_FinalRating(int client, PlayerInfo Player, PlayerInfo Opponent, MatchResults Result)
{
	float
		f_g = Glicko_g(Opponent),			 // g(RD)
		f_e = Glicko_e(Opponent, Player);	 // E(s|r,rj,RDj)

	return f_g * (g_fResult[Result] - f_e);
}

/**
 * Defines the value of r'.
 *
 * @param Player       Player profile.
 * @param d_Full       value of d.
 * @param fSum_Rating  value of Σ.
 * @return             value of r'.
 * @note               r' = r + q * d^2 * Σ( g(RD) * (s - E(s|r,rj,RDj)) )
 */
stock float Glicko_FinalRating(int client, PlayerInfo Player, float d_Full, float fSum_Rating)	  // r'
{
	float
		fGlicko_q	   = Glicko_q(),
		fGlicko_RD	   = 1.0 / Square(Glicko_RD(Player)),	 // 1 / RD^2
		fd_Full		   = 1.0 / Square(d_Full);				 // 1 / d^2

	return Player.rating + (fGlicko_q / (fGlicko_RD + fd_Full)) * fSum_Rating;
}

/**
 * Returns the square of the input value, equivalent to Pow(value, 2.0).
 *
 * @param value         Input value.
 * @return              Square of the value.
 */
stock float Square(float value)
{
	return Pow(value, 2.0);
}