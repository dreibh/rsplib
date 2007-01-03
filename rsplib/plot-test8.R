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

xAxisTicks <- seq(0,4,0.5)
yAxisTicks <- seq(0,20000,2500)


zSet <- data$Policy
zTitle <- "Pool Policy"


frameColor <- "gold4"



# Plot a histogram.
plothist <- function(mainTitle,
                     xTitle, yTitle, zTitle,
                     xSet, ySet, zSet,
                     xAxisTicks        = getUsefulTicks(xSet),
                     yAxisTicks        = c(),
                     breakSpace        = 0.2,
                     hideLegend        = FALSE,
                     legendPos         = c(1,1),
                     colorMode         = FALSE,
                     zColorArray       = c(),
                     frameColor        = par("fg"),
                     legendSizeFactor  = 0.8)
{
   # ------ Initialize variables --------------------------------------------
   zLevels <- levels(factor(zSet))
   if(length(zColorArray) == 0) {
      if(colorMode == cmColor) {
         if(length(zLevels) <= 4) {
            zColorArray <- rainbow(length(zLevels))
         }
         else {
            zColorArray <- rainbow(length(zLevels), gamma=2)
         }
      }
      else if(colorMode == cmGrayScale) {
         zColorArray <- graybow(length(zLevels))
         frameColor  <- par("fg")
      }
      else {
         zColorArray <- rep(par("fg"), length(zLevels))
         frameColor  <- par("fg")
      }
   }

   legendBackground <- "gray95"
   if(colorMode == cmBlackAndWhite) {
      legendBackground <- "white"
   }


   # ------ Initialize plot ----------------------------------------------------
   breakSet <- union(xAxisTicks, c(min(xSet), max(xSet)))

   r <- hist(xSet, br=breakSet, plot=FALSE)
   opar <- par(col.lab=frameColor,col.main=frameColor,font.main=2,cex.main=2)

   xRange <- c(min(xAxisTicks), max(xAxisTicks))
   if(length(yAxisTicks) < 2) {
      yAxisTicks <- getUsefulTicks(r$count)
   }
   yRange <- c(0, max(yAxisTicks))

   plot.new()
   plot.window(xRange, yRange)
   axis(1, xAxisTicks, col=frameColor, col.axis=frameColor)
   axis(2, yAxisTicks, col=frameColor, col.axis=frameColor)
   abline(v=xAxisTicks, lty=1, col="lightgray")
   abline(h=yAxisTicks, lty=1, col="lightgray")
   box(col=frameColor)

   mtext(xTitle, col=frameColor,
         side = 1, at = xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
         xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   mtext(yTitle, col=frameColor,
         side = 2, at = yRange[1] + ((yRange[2] - yRange[1]) / 2), line=2.5,
         xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   mtext(mainTitle, col=frameColor,
         side=3, at=xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
         xpd = NA, font = par("font.main"), cex = par("cex.main"))
   if(length(zLevels) > 1) {
      mtext(zTitle, col=frameColor,
            side = 3, at = max(xRange), line=0.5, adj=1,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }

   par(opar)


   # ------ Plot bars -------------------------------------------------------
   count <- 0
   for(zValue in zLevels) {
      xSubset <- subset(xSet, (zSet == zValue))

      r <- hist(xSubset, br=breakSet, plot=FALSE)
      brLeft <- r$breaks[1:length(breakSet) - 1]
      brRight <- r$breaks[2:length(breakSet)]
      brWidth <- brRight - brLeft
      brWidth <- brWidth - breakSpace * brWidth

      brLeft <- (0.5 * breakSpace * brWidth) + (brLeft + count * (brWidth / length(zLevels)))
      brRight <- brLeft + (brWidth / length(zLevels))

      rect(brLeft, rep(0, length(brLeft)),
         brRight, r$count,
         col=zColorArray[count + 1])

      x <- brLeft + (0.5 * brWidth / length(zLevels))
      y <- r$count
      t <- paste("",r$count,"")
      text(x[2:length(r$count)], y[2:length(r$count)],
         t[2:length(r$count)],
         adj=c(0,0),
         srt=75,
         col=zColorArray[count + 1])

      count <- count + 1
   }


   # ------ Plot legend -----------------------------------------------------
   if(!hideLegend) {
      lx <- min(xRange) + ((max(xRange) - min(xRange)) * legendPos[1])
      ly <- min(yRange) + ((max(yRange) - min(yRange)) * legendPos[2])
      lxjust <- 0.5
      lyjust <- 0.5
      if(legendPos[1] < 0.5) {
         lxjust <- 0
      }
      else if(legendPos[1] > 0.5) {
         lxjust <- 1
      }
      if(legendPos[2] < 0.5) {
         lyjust <- 0
      }
      else if(legendPos[2] > 0.5) {
         lyjust <- 1
      }

      legendBackground <- "gray95"
      if(colorMode == cmBlackAndWhite) {
         legendColors <- par("fg")
         legendBackground <- "white"
      }
      legend(lx, ly,
            xjust = lxjust,
            yjust = lyjust,
            zLevels,
            bg=legendBackground,
            col=zColorArray,
            text.col=zColorArray,
            lwd=5*par("cex"), cex=legendSizeFactor*par("cex"))
   }
}



cat("ACHTUNG: Absolute Zeit funktioniert nur bei erster Messung. Dann OFFSET notwendig!!!\n")

pdf("test7.pdf", width=10, height=10, onefile=TRUE, family="Helvetica", pointsize=18)
plothist(mainTitle, xTitle, yTitle, zTitle, xSet, ySet, zSet,
         xAxisTicks=xAxisTicks, yAxisTicks=yAxisTicks,
         frameColor=frameColor,
         colorMode=colorMode,
         hideLegend=hideLegend)
dev.off()
