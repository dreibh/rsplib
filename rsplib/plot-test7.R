source("plotter.R")


data <- loadResults("test2.data")

hideLegend <- FALSE
legendPos <- c(1,1)
colorMode <- cmColor
legendSizeFactor <- 1

mainTitle <- "Test-Histogramm"
yTitle <- "Frequency [1]"
xSet <- data$Value1
xTitle <- "Interval [s]"
zSet <- data$Type
zTitle <- "Pool Policy"
cSet <- data$Measurement

xAxisTicks <- seq(0,30,5)


frameColor <- "gold4"



pdf("test7.pdf", width=10, height=10, onefile=TRUE, family="Helvetica", pointsize=18)
plothist(mainTitle, xTitle, yTitle, zTitle, xSet, ySet, zSet, cSet,
         xAxisTicks=xAxisTicks,
         frameColor=frameColor,
         colorMode=colorMode,
         hideLegend=hideLegend,
         showMinMax=TRUE,
         showConfidence=TRUE,
         confidence=0.9)
dev.off()
