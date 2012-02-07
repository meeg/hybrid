#ifndef SHAPING_CURVE_HH
#define SHAPING_CURVE_HH

class ShapingCurve
{
	public:
		ShapingCurve(double sTime);
		~ShapingCurve();
	protected:
		double shapingTime;
	public:
		double getHeight(double time); //normalized so peak height = 1
		double getSlope(double time); //used for fitting
		double getPeak(); //used for fitting
		double getSignal(double time, int n, double *par);
};

#endif
