source("plotter.R")


# data <- loadResults("test1.data.bz2")


hideLegend <- FALSE
legendPos <- c(1,1)
colorMode <- cmColor
legendSizeFactor <- 1

mainTitle <- "Handling Time Histogram"
yTitle <- "Frequency [1]"

xSet <- data$HandlingTime
xTitle <- "Handling Time Interval [s]"

xAxisTicks <- seq(0,2.5,0.5)
yAxisTicks <- seq(0,6000,1000)


zSet <- data$Policy
zTitle <- "Pool Policy"

cSet <- data$Measurement


frameColor <- "gold4"



cat("ACHTUNG: Absolute Zeit funktioniert nur bei erster Messung. Dann OFFSET notwendig!!!\n")

pdf("test7.pdf", width=10, height=10, onefile=TRUE, family="Helvetica", pointsize=18)
plothist(mainTitle, xTitle, yTitle, zTitle, xSet, ySet, zSet, cSet,
         xAxisTicks=xAxisTicks, yAxisTicks=yAxisTicks,
         frameColor=frameColor,
         colorMode=colorMode,
         hideLegend=hideLegend,
         showMinMax=TRUE,
         showConfidence=TRUE,
         confidence=0.95)
dev.off()
