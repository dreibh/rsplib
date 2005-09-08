#ifndef STATISTICS_H
#define STATISTICS_H


class Statistics
{
   public:
   Statistics();
   ~Statistics();

   /**
    * Collects one value.
    */
   void collect(double value);

   /**
   * Returns the sum of samples collected.
   */
   inline double sum() const {
     return(sumSamples);
   }

  /**
   * Returns the squared sum of the collected data.
   */
   inline double sqrSum() const {
      return(sqSumSamples);
   }

   /**
     * Returns the minimum of the samples collected.
     */
   inline double minimum() const {
      return(minSamples);
   }

   /**
     * Returns the maximum of the samples collected.
     */
   inline double maximum() const {
      return(maxSamples);
   }

   /**
     * Returns the mean of the samples collected.
    */
   inline double mean() const {
      return(numSamples ? sumSamples/numSamples : 0.0);
   }

   /**
    * Returns the standard deviation of the samples collected.
    */
   double stddev() const;

   /**
    * Returns the variance of the samples collected.
    */
   double variance() const;


   private:
   size_t numSamples;
   double sumSamples;
   double sqSumSamples;
   double minSamples;
   double maxSamples;
};


#endif
