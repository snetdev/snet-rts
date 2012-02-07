awk '{ lines=FNR; arr[lines]=$2; sum+=$2}
     END{
       avg=sum/lines
       sum=0;
       for(i=1; i<=lines; i++) {
         v=arr[i]-avg;
      	 sum+= v*v
       }
       printf( "n=%d avg=%f  stddev=%f\n",
          lines, avg, sqrt(sum/( lines - 1)))
     }' $1
