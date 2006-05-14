source("plotter.R")

handlingTimeStat <- function(data, start, end)
{
   queuingTime <- (data$QueuingTimeStamp - min(data$QueuingTimeStamp)) / 60
   f <- (queuingTime >= start) &
        (queuingTime <= end)
   htSubset <- subset(data$HandlingTime, f)
   htMin  <- min(htSubset)
   htMax  <- max(htSubset)
   htMean <- mean(htSubset)
   htTest <- t.test(htSubset)
   cat(sep="", start, "-", end, ":   mean=", htMean,
       " +/- ", htMean - htTest$conf.int[1], "   ",
       "min=", htMin, " max=", htMax,
       "\n")
}

d1 <- loadResults("messung1/pu-vectors.vec.bz2")
cat("LU-DPF\n")
handlingTimeStat(d1, 2, 58)
d2 <- loadResults("messung2/pu-vectors.vec.bz2")
cat("LU\n")
handlingTimeStat(d2, 2, 58)

cat("-----\n")

d3 <- loadResults("messung3/pu-vectors.vec.bz2")
cat("LU-DPF\n")
handlingTimeStat(d3, 1, 14)
handlingTimeStat(d3, 16, 29)
handlingTimeStat(d3, 31, 45)
handlingTimeStat(d3, 46, 49)
handlingTimeStat(d3, 51, 64)

d4 <- loadResults("messung4/pu-vectors.vec.bz2")
cat("LU\n")
handlingTimeStat(d4, 1, 14)
handlingTimeStat(d4, 16, 29)
handlingTimeStat(d4, 31, 45)
handlingTimeStat(d4, 46, 49)
handlingTimeStat(d4, 51, 64)
