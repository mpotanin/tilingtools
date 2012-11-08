#pragma once
#include "stdafx.h"
#ifndef PROJFUNCS_H
#define PROJFUNCS_H

//#include "TileName.h"
//#include "RasterFile.h"
//#include "VectorFile.h"




class GeoReference
{
private:
	double M_PI;
	/* Ellipsoid model constants (actual values here are for WGS84) */
	double	pi;
	double	sm_a;
	double	sm_b;
	double	sm_EccSquared;
	double	UTMScaleFactor;
	double	easting;
	double	zone_adjust;
public: 
		static enum Datum
		{
			WGS84 = 1,
			KRASSOVSKY =2
		};

		static enum Projection
		{
			MERCATOR = 10,
			UTM = 11, 
			GAUSS_KRUGER = 12
		};

public:
	GeoReference ();
	~GeoReference();
private:
	//void SetGeoReference;

	/*

		This a piece of code helps to translate from lon/lat coordinates to Mercator x/y meters. 
		Also there is bonus function to calculate distance between two points on ellipsoid  

		The source for the function "from_merc_y" is Cartographic Projections Library (http://proj.maptools.org/)
		Functions "merc_x" and "merc_y" were written by Christopher Schmidt (http://lists.openstreetmap.org/pipermail/dev/2006-December/002597.html)
		
		Function "distVincenty" refers to - http://www.movable-type.co.uk/scripts/LatLongVincenty.html
	 */

	//	Converts from degrees (decimal) to radians
	double deg_rad(double ang);

	//	Converts from radians to degrees (decimal) 
	double deg_decimal(double rad);

	//	Converts from longitude to x coordinate 
	double merc_x(double lon);


	//	Converts from x coordinate to longitude 
	double from_merc_x(double x);

	//	Converts from latitude to y coordinate 
	double merc_y(double lat);


	//	Converts from y coordinate to latitude 
	double from_merc_y (double y);


private:

	void merc(double lon, double lat, double &x, double &y);

	void from_merc(double x, double y, double &lon, double &lat);


private:

	/*
	 * Calculate geodesic distance (in m) between two points specified by latitude/longitude (degrees decimal)
	 * using Vincenty inverse formula for ellipsoids (http://www.movable-type.co.uk/scripts/LatLongVincenty.html)
	 */

	/*
	function distVincenty(lon1,lat1,lon2,lat2) {

  		var p1 = new Object();
  		var p2 = new Object();
		
  		p1.lon =  deg_rad(lon1);
  		p1.lat =  deg_rad(lat1);
  		p2.lat =  deg_rad(lon2);
  		p2.lon =  deg_rad(lat2);





  		var a = 6378137, b = 6356752.3142,  f = 1/298.257223563;  // WGS-84 ellipsiod
  		var L = p2.lon - p1.lon;
  		var U1 = Math.atan((1-f) * Math.tan(p1.lat));
  		var U2 = Math.atan((1-f) * Math.tan(p2.lat));
  		var sinU1 = Math.sin(U1), cosU1 = Math.cos(U1);
  		var sinU2 = Math.sin(U2), cosU2 = Math.cos(U2);
	  
  		var lambda = L, lambdaP = 2*Math.PI;
  		var iterLimit = 20;
  		while (Math.abs(lambda-lambdaP) > 1e-12 && --iterLimit>0) {
    			var sinLambda = Math.sin(lambda), cosLambda = Math.cos(lambda);
    			var sinSigma = Math.sqrt((cosU2*sinLambda) * (cosU2*sinLambda) + 
      				(cosU1*sinU2-sinU1*cosU2*cosLambda) * (cosU1*sinU2-sinU1*cosU2*cosLambda));
    			if (sinSigma==0) return 0;  // co-incident points
    			var cosSigma = sinU1*sinU2 + cosU1*cosU2*cosLambda;
    			var sigma = Math.atan2(sinSigma, cosSigma);
    			var sinAlpha = cosU1 * cosU2 * sinLambda / sinSigma;
    			var cosSqAlpha = 1 - sinAlpha*sinAlpha;
    			var cos2SigmaM = cosSigma - 2*sinU1*sinU2/cosSqAlpha;
    			if (isNaN(cos2SigmaM)) cos2SigmaM = 0;  // equatorial line: cosSqAlpha=0 (§6)
    			var C = f/16*cosSqAlpha*(4+f*(4-3*cosSqAlpha));
    			lambdaP = lambda;
    			lambda = L + (1-C) * f * sinAlpha *
      				(sigma + C*sinSigma*(cos2SigmaM+C*cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)));
  		}
  		if (iterLimit==0) return NaN  // formula failed to converge

  		var uSq = cosSqAlpha * (a*a - b*b) / (b*b);
  		var A = 1 + uSq/16384*(4096+uSq*(-768+uSq*(320-175*uSq)));
  		var B = uSq/1024 * (256+uSq*(-128+uSq*(74-47*uSq)));
  		var deltaSigma = B*sinSigma*(cos2SigmaM+B/4*(cosSigma*(-1+2*cos2SigmaM*cos2SigmaM)-
    			B/6*cos2SigmaM*(-3+4*sinSigma*sinSigma)*(-3+4*cos2SigmaM*cos2SigmaM)));
  		var s = b*A*(sigma-deltaSigma);
	  
  		s = s.toFixed(3); // round to 1mm precision
  		return s;
	}
	*/



////////////////////////////////////////////////////////////////////////
///////////////////////////////UTM logic////////////////////////////////						
////////////////////////////////////////////////////////////////////////
//BOOL	bWGS84Datum = TRUE;



	/*
	* DegToRad
	*
	* Converts degrees to radians.
	*
	*/
	double DegToRad (double deg);


	/*
	* RadToDeg
	*
	* Converts radians to degrees.
	*
	*/
	double RadToDeg (double rad);




	/*
	* ArcLengthOfMeridian
	*
	* Computes the ellipsoidal distance from the equator to a point at a
	* given latitude.
	*
	* Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
	* GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
	*
	* Inputs:
	*     phi - Latitude of the point, in radians.
	*
	* Globals:
	*     sm_a - Ellipsoid model major axis.
	*     sm_b - Ellipsoid model minor axis.
	*
	* Returns:
	*     The ellipsoidal distance of the point from the equator, in meters.
	*
	*/
	double ArcLengthOfMeridian (double phi);



		/*
		* UTMCentralMeridian
		*
		* Determines the central meridian for the given UTM zone.
		*
		* Inputs:
		*     zone - An integer value designating the UTM zone, range [1,60].
		*
		* Returns:
		*   The central meridian for the given UTM zone, in radians, or zero
		*   if the UTM zone parameter is outside the range [1,60].
		*   Range of the central meridian is the radian equivalent of [-177,+177].
		*
		*/
private:

	double UTMCentralMeridian (double zone);

private:

	/*
	* FootpointLatitude
	*
	* Computes the footpoint latitude for use in converting transverse
	* Mercator coordinates to ellipsoidal coordinates.
	*
	* Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
	*   GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
	*
	* Inputs:
	*   y - The UTM northing coordinate, in meters.
	*
	* Returns:
	*   The footpoint latitude, in radians.
	*
	*/
	double FootpointLatitude (double y);

	/*
	* MapLatLonToXY
	*
	* Converts a latitude/longitude pair to x and y coordinates in the
	* Transverse Mercator projection.  Note that Transverse Mercator is not
	* the same as UTM; a scale factor is required to convert between them.
	*
	* Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
	* GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
	*
	* Inputs:
	*    phi - Latitude of the point, in radians.
	*    lambda - Longitude of the point, in radians.
	*    lambda0 - Longitude of the central meridian to be used, in radians.
	*
	* Outputs:
	*    xy - A 2-element array containing the x and y coordinates
	*         of the computed point.
	*
	* Returns:
	*    The function does not return a value.
	*
	*/
	void MapLatLonToXY (double phi, double lambda, double lambda0, double &x, double &y);
	    
	    
	    
		/*
		* MapXYToLatLon
		*
		* Converts x and y coordinates in the Transverse Mercator projection to
		* a latitude/longitude pair.  Note that Transverse Mercator is not
		* the same as UTM; a scale factor is required to convert between them.
		*
		* Reference: Hoffmann-Wellenhof, B., Lichtenegger, H., and Collins, J.,
		*   GPS: Theory and Practice, 3rd ed.  New York: Springer-Verlag Wien, 1994.
		*
		* Inputs:
		*   x - The easting of the point, in meters.
		*   y - The northing of the point, in meters.
		*   lambda0 - Longitude of the central meridian to be used, in radians.
		*
		* Outputs:
		*   philambda - A 2-element containing the latitude and longitude
		*               in radians.
		*
		* Returns:
		*   The function does not return a value.
		*
		* Remarks:
		*   The local variables Nf, nuf2, tf, and tf2 serve the same purpose as
		*   N, nu2, t, and t2 in MapLatLonToXY, but they are computed with respect
		*   to the footpoint latitude phif.
		*
		*   x1frac, x2frac, x2poly, x3poly, etc. are to enhance readability and
		*   to optimize computations.
		*
		*/
	void MapXYToLatLon (double x, double y, double lambda0, double &phi, double &lambda);


private:

	/*
	* LatLonToUTMXY
	*
	* Converts a latitude/longitude pair to x and y coordinates in the
	* Universal Transverse Mercator projection.
	*
	* Inputs:
	*   lat - Latitude of the point, in radians.
	*   lon - Longitude of the point, in radians.
	*   zone - UTM zone to be used for calculating values for x and y.
	*          If zone is less than 1 or greater than 60, the routine
	*          will determine the appropriate zone from the value of lon.
	*
	* Outputs:
	*   xy - A 2-element array where the UTM x and y values will be stored.
	*
	* Returns:
	*   The UTM zone used for calculating the values of x and y.
	*
	*/
	double LatLonToUTMXY (double lat, double lon, double zone, double &x, double &y);
	/*
	* UTMXYToLatLon
	*
	* Converts x and y coordinates in the Universal Transverse Mercator
	* projection to a latitude/longitude pair.
	*
	* Inputs:
	*	x - The easting of the point, in meters.
	*	y - The northing of the point, in meters.
	*	zone - The UTM zone in which the point lies.
	*	southhemi - True if the point is in the southern hemisphere;
	*               false otherwise.
	*
	* Outputs:
	*	latlon - A 2-element array containing the latitude and
	*            longitude of the point, in radians.
	*
	* Returns:
	*	The function does not return a value.
	*
	*/
	void UTMXYToLatLon (double x, double y, double zone, double southhemi, double &lat, double &lon);
	////////////////////////////////////////////////////////////////////////
	///////////////////////////////Gauss-Kruger logic///////////////////////						
	////////////////////////////////////////////////////////////////////////
	/* Ellipsoid model constants (actual values here are for WGS84) */
	//double pi = 3.14159265358979;


	/*
	double GKsm_a = 6378245.0;
	double GKsm_b = 6356863.019;
	double GKsm_EccSquared = 6.693421552039e-03;

	double GKScaleFactor = 1.;
	*/

private:
	
	void SetMercatorProjection ();
	void SetUTMProjection ();
	void SetGKProjection ();
	void SetKrassovskyDatum ();
	void SetWGS84Datum();
public:

	void SetProjection  (Projection ProjName, int nZone = 0);
	void SetDatum		(Datum DatumName);
	void ToLatLon(double x, double y, double &lon, double &lat);
	void ToXY	(double lon, double lat, double &x, double &y);
	int CalcZoneNum	(double lon);
	int GetZoneNum ();
	int CalcCentrMrdn	(int nZone);
	BOOL WriteTabFile	(wstring strRasterFileName, int nWidth, int nHeight, double dULx, double dULy, double dRes, wstring strTabFileName);


	BOOL WriteWldFile	(double dULx, double dULy, double dRes, wstring strWldFileName);
	BOOL WriteMapFile	(wstring strRasterFileName, int nWidth, int nHeight, double dULx, double dULy, double dRes, wstring strMapFileName);
	BOOL WritePrjFile	(wstring strPrjFile);
	BOOL WriteAuxFile	(wstring strAuxFile);
	BOOL WriteKmlFile	(wstring strKmlFile, wstring strTitle, wstring strJpegFile, double north, double west, double south, double east);

private:
	
	BOOL SetOGRSpatialReference ();
private:
		//string	m_strProjection;
		//string	m_strDatum;
		//string
		int			m_nZone;
		OGRSpatialReference m_oOGRSpatial;
		Projection	m_Projection;
		Datum		m_Datum;


};



#endif