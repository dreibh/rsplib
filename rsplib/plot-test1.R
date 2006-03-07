source("plotter.R")


simulationDirectory <- "."
simulationName      <- "Influence of the PU:PE Ratio"
hideLegend          <- FALSE
colorMode           <- cmColor
pStart              <- 1
legendPos           <- c(1,1)


#pdf("test1.pdf", width=11.69, height=8.26, onefile=TRUE, family="Helvetica", pointsize=14)


data <- loadResults("v0.vec")

xSet <- data$CompleteTimeStamp
xTitle <- "Completion Time"
ySet <- data$HandlingSpeed
yTitle <- "Request Handling Speed"

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


xAxisTicks <- c() #getIntegerTicks(xSet)   # Set to c() for automatic setting
yAxisTicks <- getIntegerTicks(ySet, count=10)   # Set to c() for automatic setting

mainTitle <- "Titel"

hbarSet <- data$StartupTimeStamp
plotstd6(mainTitle, pTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                     pSet, aSet, bSet, xSet, ySet, zSet,
                     vSet, wSet, vTitle, wTitle,
                     xAxisTicks=xAxisTicks,yAxisTicks=yAxisTicks,
                     type="h",
                     colorMode=colorMode, hideLegend=hideLegend, pStart=pStart,
                     simulationName=simulationDirectory, legendPos=legendPos)

#dev.off()
