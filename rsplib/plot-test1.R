source("plotter.R")


simulationDirectory <- "."
simulationName      <- "Influence of the PU:PE Ratio"
hideLegend          <- TRUE
colorMode           <- cmColor
pStart              <- 1
legendPos           <- c(1,1)


pdf("test2.pdf", width=11.69, height=8.26, onefile=TRUE, family="Helvetica", pointsize=14)


data <- loadResults("messung2/pu-vectors.vec.bz2")

xSet <- data$CompleteTimeStamp
xOffset <- -min(xSet)
xSet <- xSet + xOffset

xTitle <- "Request Completion Time Stamp [s]"
ySet <- data$HandlingSpeed
yTitle <- "Request Handling Speed [Calculations/s]"
#yTitle <- "Request Handling Time [s]"

zSet <- data$ObjectName
zTitle <- "PU"
vSet <- c()
vTitle <- ""
wSet <- c()
wTitle <- ""

bSet <- c()
bTitle <- ""
aSet <- c()
aTitle <- ""

pSet <- c()
pTitle <- ""


xAxisTicks <- seq(0, 3600, 300)
# c() #getIntegerTicks(xSet)   # Set to c() for automatic setting
yAxisTicks <- getIntegerTicks(ySet, count=10)   # Set to c() for automatic setting

mainTitle <- "Request Handling Performance"

hbarSet <- data$QueuingTimeStamp + xOffset
plotstd6(mainTitle, pTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                     pSet, aSet, bSet, xSet, ySet, zSet,
                     vSet, wSet, vTitle, wTitle,
                     xAxisTicks=xAxisTicks,yAxisTicks=yAxisTicks,
                     type="h",
                     colorMode=colorMode, hideLegend=hideLegend, pStart=pStart,
                     simulationName=simulationDirectory, legendPos=legendPos)

dev.off()
