#include "SmoothShapingCurve.hh"

#include <math.h>


SmoothShapingCurve::SmoothShapingCurve(double sTime) : ShapingCurve(sTime)
{
	ShapingCurve *templateCurve = new ShapingCurve(sTime);
	double x[100], y[100];
	int i;
	for (i=0;i<100;i++)
	{
		x[i] = 0.1*sTime*(i-20);
		y[i] = templateCurve->getHeight(0.1*sTime*(i-20));
	}
	theSpline = new TSpline3("curve",x,y,100,"");
}

SmoothShapingCurve::~SmoothShapingCurve()
{
}

double SmoothShapingCurve::getHeight(double time)
{
	return theSpline->Eval(time);
	/*
	if (time<0) return 0;
	else return (time/shapingTime)*exp(1.0-time/shapingTime);
	*/
}

double SmoothShapingCurve::getSlope(double time)
{
	return theSpline->Derivative(time);
	/*
	if (time<-0.2*shapingTime) return 0;
	else if (time<0) return (1.0+time/(0.2*shapingTime))*exp(1.0)/shapingTime;
	else return (1.0-time/shapingTime)*exp(1.0-time/shapingTime)/shapingTime;
	*/
}
