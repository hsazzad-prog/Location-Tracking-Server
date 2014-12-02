//Coding for calculating distance by inside the point//
//Design by Nur Haliza//
//TRY and ERROR version//


#include <stdio.h>
#include <math.h>

int main()
{
 	float Xmn,Ymn,Ymn1,Ymn2,Xinput,Yinput,alfa_xy,Distance,dp1,dp2;

for(;;){

printf("enter referend point x,y : " );
scanf  ("%f,%f", &Xinput, &Yinput); //key in location of other point
printf("enter alfa: " );
scanf  ("%f", &alfa_xy); //inside alfa value

 	Xmn=3*cos(alfa_xy); //assume a=3 for try the coding 
 	Ymn1=5*sin(alfa_xy); Ymn2=-Ymn1; //assume b=5
	dp1=sqrt(Xmn*Xmn + (10-Ymn1)*(10-Ymn1));
        dp2=sqrt(Xmn*Xmn + (10-Ymn2)*(10-Ymn2));
        if (fabsf(dp1-2) < fabsf(dp2-2)) //assume c=2
 	{ 	
 	Ymn=Ymn1;
 	Distance=sqrt((Xmn-Xinput)*(Xmn-Xinput)+(Ymn1-Yinput)*(Ymn1-Yinput)); //measure the distance
 	}
        else
        {	 
 	Ymn=Ymn2;
 	Distance=sqrt((Xmn-Xinput)*(Xmn-Xinput)+(Ymn2-Yinput)*(Ymn2-Yinput)); //measure the distance
  	}
	    
printf ("You enter %f, %f and alfa is %f \n", Xinput, Yinput, alfa_xy);
printf("alfa is %f, dp1 is %f,dp2 is %f. The location of mobile node is (%f,%f). The distance between mobile node (%f,%f) and (%f,%f) is %f \n",alfa_xy,dp1,dp2,Xmn,Ymn, Xinput, Yinput, Xmn, Ymn, Distance);
}
}


//comment .......... the distance value in the result display = NOT CONSISTENT! ............... WHY??????   21 April 09, 2:49 am


//comment .......... the distance value in the result dislay = CONSISTENT ANSWER! ......... BUT ...... the answer of the calculation is not true! 21 April 09, 3:33 am

//coment ..... find out! the value of alfa is in RADIAN! so, this coding can work! 21 April 09, 3:43 am.
