source("plotter.R")

d1 <- loadResults("messung1/pu-vectors.vec.bz2")
d2 <- loadResults("messung2/pu-vectors.vec.bz2")
#d3 <- loadResults("messung3/pu-vectors.vec.bz2")

cat(sep="", "D1.HandlingTime=", mean(d1$HandlingTime), "s\n")
cat(sep="", "D2.HandlingTime=", mean(d2$HandlingTime), "s\n")
#cat(sep="", "D3.HandlingTime=", mean(d3$HandlingTime), "s\n")
