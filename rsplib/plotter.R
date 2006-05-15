# ###########################################################################
#             Thomas Dreibholz's R Simulation Scripts Collection
#                 Copyright (C) 2005-2006 Thomas Dreibholz
#
#           Author: Thomas Dreibholz, dreibh@exp-math.uni-essen.de
# ###########################################################################


# Get array of gray tones (equivalent of rainbow() for b/w laser printing)
graybow <- function(n)
{
   maxGray <- 0.5   # Light gray
   minGray <- 0.0   # Black
   return(gray(seq(maxGray, minGray, -((maxGray - minGray) / n))))
}


# Get background color
cmColor <- 2
cmGrayScale <- 1
cmBlackAndWhite <- 0
getBackgroundColor <- function(index, colorMode = cmColor, pStart = 0)
{
   if(colorMode == cmColor) {
      bgColorSet <- c("#ffffe0", "#ffe0ff", "#e0ffff", "#ffe0e0", "#e0ffe0", "#e0e0ff", "#e0e0c0", "#eeeeee", "#000000")
   }
   else if(colorMode == cmGrayScale) {
      bgColorSet <- c("#ffffff", "#f8f8f8", "#f0f0f0", "#e8e8e8", "#e0e0e0", "#d8d8d8", "#d0d0d0", "#c8c8c8", "#000000")
   }
   else {
      bgColorSet <- c("#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff", "#ffffff")
   }

   bgColor <- bgColorSet[1 + ((pStart + index) %% length(bgColorSet))]
   return(bgColor)
}


titleRegExpr <- "([^\[\{]*)([\{]([^\}]*)[\}]){0,1}([^\[]*)([\[]([^\]]*)[\]]){0,1}"
removeSpaceRegExpr <- "([[:space:]]*)([^ ]*)([[:space:]]*)"


# Extract variable from axis title
getVariable <- function(title)
{
   result <- sub(extended=TRUE, titleRegExpr, "\\1", title)
   return(result)
}


# Extract abbreviated variable name from axis title
getAbbreviation <- function(title)
{
   result <- sub(extended=TRUE, titleRegExpr, "\\3", title)
   if(result == "") {
      return(getVariable(title))
   }
   return(result)
}


# Extract unit from axis title
getUnit <- function(title)
{
   result <- sub(extended=TRUE, titleRegExpr, "\\6", title)
   return(result)
}




# Check sets
checkSets <- function(data,
                      xSet=c(), ySet=c(), zSet=c(),
                      vSet=c(), wSet=c(),
                      aSet=c(), bSet=c(), pSet=c(),
                      runNoSet=c())
{
   if(length(xSet) < 1) {
      stop("checkSets: xSet is empty!")
   }
   if( (length(xSet) != length(ySet)) ) {
      stop("checkSets: xSet and ySet length differ!")
   }
   if( (length(zSet) > 0) && (length(xSet) != length(zSet)) ) {
      stop("checkSets: xSet and zSet length differ!")
   }
   if( (length(vSet) > 0) && (length(xSet) != length(vSet)) ) {
      stop("checkSets: xSet and vSet length differ!")
   }
   if( (length(wSet) > 0) && (length(xSet) != length(wSet)) ) {
      stop("checkSets: xSet and wSet length differ!")
   }
   if( (length(aSet) > 0) && (length(xSet) != length(aSet)) ) {
      stop("checkSets: xSet and aSet length differ!")
   }
   if( (length(bSet) > 0) && (length(xSet) != length(bSet)) ) {
      stop("checkSets: xSet and bSet length differ!")
   }
   if( (length(pSet) > 0) && (length(xSet) != length(pSet)) ) {
      stop("checkSets: xSet and pSet length differ!")
   }

   if(length(runNoSet) > 0) {
      runs <- length(levels(factor(runNoSet)))

      zFilter <- TRUE
      if(length(zSet) > 0) {
         zFilter <- (zSet == zSet[1])
      }
      vFilter <- TRUE
      if(length(vSet) > 0) {
         vFilter <- (vSet == vSet[1])
      }
      wFilter <- TRUE
      if(length(wSet) > 0) {
         wFilter <- (wSet == wSet[1])
      }
      aFilter <- TRUE
      if(length(aSet) > 0) {
         aFilter <- (aSet == aSet[1])
      }
      bFilter <- TRUE
      if(length(bSet) > 0) {
         bFilter <- (bSet == bSet[1])
      }
      pFilter <- TRUE
      if(length(pSet) > 0) {
         pFilter <- (pSet == pSet[1])
      }

      filter <- (xSet == xSet[1]) &
               zFilter & vFilter & wFilter & aFilter & bFilter & pFilter
      ySubset <- subset(data, filter)
      n <- length(ySubset)

      if(n != runs) {
         stop("checkSets: Number of value differs from number of runs!")
      }
   }
}


# Plot x/y plot with different curves as z with confidence intervals in
# y direction. x and z can be numeric or strings, y must be numeric since
# confidence intervals have to be computed.
plotstd3 <- function(mainTitle,
                     xTitle, yTitle, zTitle,
                     xSet, ySet, zSet,
                     vSet              = c(),
                     wSet              = c(),
                     vTitle            = "??vTitle??",
                     wTitle            = "??wTitle??",
                     xAxisTicks        = c(),
                     yAxisTicks        = c(),
                     confidence        = 0.95,
                     hbarSet           = c(),
                     hbarMeanSteps     = 10,
                     xSeparatorsSet    = c(),
                     xSeparatorsTitles = c(),
                     xSeparatorsColors  = c(),
                     type              = "lines",
                     hideLegend        = FALSE,
                     legendOnly        = FALSE,
                     legendPos         = c(0,1),
                     colorMode         = FALSE,
                     frameColor        = par("fg"))
{
   xLevels <- levels(factor(xSet))
   yLevels <- levels(factor(ySet))
   zLevels <- levels(factor(zSet))
   if(length(xLevels) < 1) {
      cat("WARNING: plotstd3() - xLevels=c()\n")
      return(0);
   }
   if(length(yLevels) < 1) {
      cat("WARNING: plotstd3() - yLevels=c()\n")
      return(0);
   }
   if(length(zLevels) < 1) {
      cat("WARNING: plotstd3() - zLevels=c()\n")
      return(0);
   }
   if(length(vSet) < 1) {
      vSet <- rep(0, length(zSet))
   }
   vLevels <- levels(factor(vSet))
   if(length(wSet) < 1) {
      wSet <- rep(0, length(zSet))
   }
   wLevels <- levels(factor(wSet))

   if(colorMode == cmColor) {
      zColorArray <- rainbow(length(zLevels))
   }
   else if(colorMode == cmGrayScale) {
      zColorArray <- graybow(length(zLevels))
      frameColor  <- par("fg")
   }
   else {
      zColorArray <- rep(par("fg"), length(zLevels))
      frameColor  <- par("fg")
   }

   xRange <- range(as.numeric(xSet), finite=TRUE)
   yRange <- range(as.numeric(ySet), finite=TRUE)
   if(missing(xAxisTicks) || (length(xAxisTicks) == 0) || nlevels(xSet)) {
      xAxisTicks <- xLevels
   }
   else {
      xRange <- range(as.numeric(xAxisTicks), finite=TRUE)
   }
   if(missing(yAxisTicks) || (length(yAxisTicks) == 0)) {
      yAxisTicks <- yLevels
   }
   else {
      yRange <- range(as.numeric(yAxisTicks), finite=TRUE)
   }


   # ------ Create plot window ----------------------------------------------
   plot.new()
   plot.window(xRange, yRange)
   if(!legendOnly) {
      if(nlevels(xSet)) {
         axis(1, seq(length(xLevels)), xLevels, col=frameColor, col.axis=frameColor)
      }
      else {
         axis(1, xAxisTicks, col=frameColor, col.axis=frameColor)
      }
      axis(2, yAxisTicks, col=frameColor, col.axis=frameColor)

      grid(20, 20)
      box(col=frameColor)
      xLabel <- getVariable(xTitle)
      if(getAbbreviation(xTitle) != getVariable(xTitle)) {
         xLabel <- paste(sep="", xLabel, " ", getAbbreviation(xTitle), "")
      }
      if(getUnit(xTitle) != "") {
         xLabel <- paste(sep="", xLabel, " [", getUnit(xTitle), "]")
      }
      yLabel <- getVariable(yTitle)
      if(getAbbreviation(yTitle) != getVariable(yTitle)) {
         yLabel <- paste(sep="", yLabel, " ", getAbbreviation(yTitle), "")
      }
      if(getUnit(yTitle) != "") {
         yLabel <- paste(sep="", yLabel, " [", getUnit(yTitle), "]")
      }

      mtext(xLabel, col=frameColor,
            side = 1, at = xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
      mtext(yLabel, col=frameColor,
            side = 2, at = yRange[1] + ((yRange[2] - yRange[1]) / 2), line=2.5,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
      mtext(mainTitle, col=frameColor,
            side=3, at=xRange[1] + ((xRange[2] - xRange[1]) / 2), line=2.5,
            xpd = NA, font = par("font.main"), cex = par("cex.main"))

      zvwLabel <- getVariable(zTitle)
      if(getAbbreviation(zTitle) != getVariable(zTitle)) {
         zvwLabel <- paste(sep="", zvwLabel, " ", getAbbreviation(zTitle), "")
      }
      if(getUnit(zTitle) != "") {
         zvwLabel <- paste(sep="", zvwLabel, " [", getUnit(zTitle), "]")
      }
      if(length(vLevels) > 1) {
         zvwLabel <- paste(sep="", zvwLabel, " / ", getVariable(vTitle))
         if(getAbbreviation(vTitle) != getVariable(vTitle)) {
            zvwLabel <- paste(sep="", zvwLabel, " ", getAbbreviation(vTitle), "")
         }
         if(getUnit(vTitle) != "") {
            zvwLabel <- paste(sep="", zvwLabel, " [", getUnit(vTitle), "]")
         }
      }
      if(length(wLevels) > 1) {
         zvwLabel <- paste(sep="", zvwLabel, " / ", getVariable(wTitle))
         if(getAbbreviation(wTitle) != getVariable(wTitle)) {
            zvwLabel <- paste(sep="", zvwLabel, " ", getAbbreviation(wTitle), "")
         }
         if(getUnit(wTitle) != "") {
            zvwLabel <- paste(sep="", zvwLabel, " [", getUnit(wTitle), "]")
         }
      }
      mtext(zvwLabel, col=frameColor,
            side = 3, at = max(xRange), line=0.5, adj=1,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }


   # ------ Plot curves -----------------------------------------------------
   legendTexts  <- c()
   legendColors <- c()
   legendStyles <- c()
   legendDots   <- c()
   legendDot    <- 1
   for(zPosition in rev(seq(1, length(zLevels)))) {
      z <- zLevels[zPosition]
      for(wPosition in seq(1, length(wLevels))) {
         w <- wLevels[wPosition]
         for(vPosition in seq(1, length(vLevels))) {
            v <- vLevels[vPosition]

            # ----- Legend settings -----------------------------------------
            if((length(vLevels) > 1) ||
               (length(wLevels) > 1)) {
               legendText <- paste(sep="", getAbbreviation(zTitle), "=", z)
            }
            else {
               legendText  <- paste(sep="", z)
            }
            if(length(vLevels) > 1) {
               legendText <- paste(sep="", legendText, ", ", getAbbreviation(vTitle), "=", v)
            }
            if(length(wLevels) > 1) {
               legendText <- paste(sep="", legendText, ", ", getAbbreviation(wTitle), "=", w)
            }
            legendColor <- zColorArray[zPosition]
            legendStyle <- vPosition %% 7

            if(!legendOnly) {
               # ----- Points plot ------------------------------------------
               if((type == "p") || (type=="points")) {
                  xSubset <- subset(xSet, (zSet == z) & (vSet == v) & (wSet == w))
                  ySubset <- subset(ySet, (zSet == z) & (vSet == v) & (wSet == w))
                  points(xSubset, ySubset, col=legendColor, cex=par("cex"), pch=legendDot)

                  legendTexts <- append(legendTexts, legendText)
                  legendColors <- append(legendColors, legendColor)
                  legendStyles <- append(legendStyles, legendStyle)
                  legendDots <- append(legendDots, legendDot)
               }

               # ----- Horizontal bars plot ---------------------------------
               else if((type == "h") || (type=="hbars")) {
                  xSubset <- subset(xSet, (zSet == z) & (vSet == v) & (wSet == w))
                  ySubset <- subset(ySet, (zSet == z) & (vSet == v) & (wSet == w))
                  hbarSubset <- subset(hbarSet, (zSet == z) & (vSet == v) & (wSet == w))
                  for(x in seq(1, length(xSubset))) {
                     # points(xSubset[x],ySubset[x], col=legendColor, cex=par("cex"), pch=legendDot)
                     lines(c(hbarSubset[x], xSubset[x]),
                           c(ySubset[x], ySubset[x]),
                           col=legendColor, cex=par("cex"), pch=legendDot)
                  }
                  legendTexts <- append(legendTexts, legendText)
                  legendColors <- append(legendColors, legendColor)
                  legendStyles <- append(legendStyles, legendStyle)
                  legendDots <- append(legendDots, legendDot)
               }

               # ----- Lines plot -------------------------------------------
               else if((type == "l") || (type=="lines")) {
                  # These sets will contain mean y line and its confidence intervals
                  xPlotSet <- c()
                  yPlotMinSet <- c()
                  yPlotMeanSet <- c()
                  yPlotMaxSet <- c()
                  for(xPosition in seq(1, length(xLevels))) {
                     x <- xLevels[xPosition]

                     # ------ Calculate confidence intervals for (z,x) pos. ----
                     ySubset <- subset(ySet, (zSet == z) & (vSet == v) & (wSet == w) & (xSet == x))
                     if(length(ySubset) > 0) {
                        yMin <- min(ySubset)
                        yMean <- mean(ySubset)
                        yMax <- max(ySubset)
                        if(yMin != yMax) {
                           yTest <- t.test(ySubset, conf.level=confidence)
                           yMin <- yTest$conf.int[1]
                           yMax <- yTest$conf.int[2]
                        }

                        # ------ Add results to set ----------------------------
                        if(nlevels(xSet)) {
                           xPlotSet <- append(xPlotSet, xPosition)
                        }
                        else {
                           xPlotSet <- append(xPlotSet, as.numeric(x))
                        }
                        yPlotMinSet  <- append(yPlotMinSet, yMin)
                        yPlotMeanSet <- append(yPlotMeanSet, yMean)
                        yPlotMaxSet  <- append(yPlotMaxSet, yMax)
                     }
                  }

                  # ------ Plot line and confidence intervals ---------------
                  if(length(xPlotSet) > 0) {
                     lineWidth <- 5
                     if((length(vLevels) > 1) || (length(wLevels) > 1)) {
                        lineWidth <- 3
                     }
                     lines(xPlotSet, yPlotMeanSet,
                           col=legendColor, lty=legendStyle, lwd=lineWidth*par("cex"))

                     cintWidthFraction <- 75
                     cintWidth <- (max(xRange) - min(xRange)) / cintWidthFraction
                     for(xPosition in seq(1, length(xPlotSet))) {
                        x <- xPlotSet[xPosition]
                        lines(c(x, x),
                              c(yPlotMinSet[xPosition], yPlotMaxSet[xPosition]),
                              col=legendColor, lty=legendStyle, lwd=1*par("cex"))
                        lines(c(x - cintWidth, x + cintWidth),
                              c(yPlotMinSet[xPosition], yPlotMinSet[xPosition]),
                              col=legendColor, lty=legendStyle, lwd=1*par("cex"))
                        lines(c(x - cintWidth, x + cintWidth),
                              c(yPlotMaxSet[xPosition],yPlotMaxSet[xPosition]),
                              col=legendColor, lty=legendStyle, lwd=1*par("cex"))
                     }

                     legendTexts <- append(legendTexts, legendText)
                     legendColors <- append(legendColors, legendColor)
                     legendStyles <- append(legendStyles, legendStyle)
                     legendDots <- append(legendDots, legendDot)

                     pcex <- par("cex")
                     # if(length(wLevels) > 1) {
                        pcex <- 2 * pcex
                     # }
                     points(xPlotSet, yPlotMeanSet,
                           col=legendColor, lty=legendStyle, pch=legendDot,
                           lwd=par("cex"),
                           cex=pcex,bg="yellow")
                     legendDot <- legendDot + 1
                  }
               }

               # ----- Unknown plot type ------------------------------------
               else {
                  stop("plotstd3: Unknown plot type!")
               }
            }
         }
      }
   }


   # ----- Plot hbar mean line ----------------------------------------------
   if( ((type == "h") || (type=="hbars")) &&
       (hbarMeanSteps > 1) ) {
      xSteps <- hbarMeanSteps
      xWidth <- ((max(xSet) - min(xSet)) / xSteps)
      xSegments <- seq(min(xSet), max(xSet) - xWidth, by=xWidth)

      oldY <- c()
      for(xValue in xSegments) {
         mSet <- subset(ySet, (xSet >= xValue) &
                              (xSet <= xValue + xWidth))
         mMean <- mean(mSet)
         mMin <- min(mSet)
         mMax <- max(mSet)
         if(mMin != mMax) {
            mTest <- t.test(mSet, conf.level=confidence)
            mMin  <- mTest$conf.int[1]
            mMax  <- mTest$conf.int[2]
         }

         # ------ Plot line segment -----------------------------------------
         if(colorMode == cmColor) {
            meanBarColor <- zColorArray
         }
         else {
            meanBarColor <- par("fg")
         }
         if(xValue > min(xSet)) {
            lines(c(xValue, xValue),
                  c(oldY, mMean),
                  col=meanBarColor, lwd=4*par("cex"))
         }
         lines(c(xValue, xValue + xWidth),
               c(mMean, mMean),
               col=meanBarColor, lwd=4*par("cex"))
         oldY <- mean(mSet)

         # ------ Plot confidence interval ----------------------------------
         x <- xValue + (xWidth / 2)
         cintWidthFraction <- 75
         cintWidth <- (max(xSet) - min(ySet)) / cintWidthFraction
         lines(c(x, x), c(mMin, mMax),
               col=meanBarColor, lwd=1*par("cex"))
         lines(c(x - cintWidth, x + cintWidth),
               c(mMin, mMin),
               col=meanBarColor, lwd=1*par("cex"))
         lines(c(x - cintWidth, x + cintWidth),
               c(mMax, mMax),
               col=meanBarColor, lwd=1*par("cex"))
      }
   }


   # ------ Plot separators -------------------------------------------------
   if(length(xSeparatorsSet) > 0) {
      if(length(xSeparatorsColors) < 1) {
         if(colorMode == cmColor) {
            xSeparatorsColors <- rainbow(length(xSeparatorsSet))
         }
         else if(colorMode == cmGrayScale) {
            xSeparatorsColors <- graybow(length(xSeparatorsSet))
         }
         else {
            xSeparatorsColors <- rep(par("fg"), length(zLevels))
            frameColor  <- par("fg")
         }
      }
      legendBackground <- "gray95"
      if(colorMode == cmBlackAndWhite) {
         legendBackground <- "white"
      }

      i <- 1
      for(xValue in xSeparatorsSet) {
         lines(c(xValue, xValue),
               c(-9e99, 9e99), lwd=4*par("cex"),
               col=xSeparatorsColors[i])
         xAdjust <- -(strwidth(xSeparatorsTitles[i]) / 2)
         rect(xValue + xAdjust, max(yAxisTicks),
              xValue + xAdjust + strwidth(xSeparatorsTitles[i]) + 2 * strwidth("i"),
              max(yAxisTicks) - strheight(xSeparatorsTitles[i]) - 1.0 * strheight("i"),
              col=legendBackground)
         text(xValue + strwidth("i"),
              max(yAxisTicks) - 0.5 * strheight(xSeparatorsTitles[i]) - 0.5 * strheight("i"),
              xSeparatorsTitles[i],
              col=xSeparatorsColors[i],
              adj=c(0.5,0.5))
         i <- i + 1
      }
   }


   # ------ Plot legend -----------------------------------------------------
   if(!hideLegend) {
      lx <- (max(xRange) - min(xRange)) * legendPos[1]
      ly <- (max(yRange) - min(yRange)) * legendPos[2]
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
             legendTexts,
             bg=legendBackground,
             col=legendColors,
             lty=legendStyles,
             pch=legendDots,
             text.col=legendColors,
             lwd=par("cex"), cex=0.8*par("cex"))
   }
}


# 2-dimensional array of plotstd3 plots (a-dimension only)
plotstd4 <- function(mainTitle, aTitle, xTitle, yTitle, zTitle,
                     aSet, xSet, ySet, zSet,
                     vSet=c(), wSet=c(),
                     vTitle     = "??vTitle??",
                     wTitle     = "??wTitle??",
                     aAxisTicks = length(levels(factor(aSet))) > 1,
                     xAxisTicks = c(),
                     yAxisTicks = c(),
                     type       = "lines",
                     hideLegend = FALSE,
                     legendPos  = c(0,1),
                     colorMode  = cmColor,
                     frameColor = par("fg"))
{
   aLevels <- levels(factor(aSet))
   aLevelsCount <- length(aLevels)
   if(length(aLevels) < 1) {
      cat("WARNING: plotstd4() - aLevels=c()\n")
      return(0);
   }

   if(colorMode == cmColor) {
      aLevelsColorArray <- rainbow(length(aLevels), start=0, end=3/6)
   }
   else if(colorMode == cmGrayScale) {
      aLevelsColorArray <- graybow(length(aLevels))
   }
   else {
      aLevelsColorArray <- rep(par("fg"), length(aLevels))
   }

   width  <- ceiling(sqrt(aLevelsCount))
   height <- ceiling(aLevelsCount / width)


   # ------ Get x/y tickmarks -----------------------------------------------
   if(missing(xAxisTicks) || (length(xAxisTicks) == 0)) {
      xAxisTicks = levels(factor(xSet))
      if(length(xAxisTicks) > 20) {
         unique(seq(min(xSet), max(xSet), length=10))
      }
   }
   if(missing(yAxisTicks) || (length(yAxisTicks) == 0)) {
      yAxisTicks = levels(factor(ySet))
      if(length(yAxisTicks) > 20) {
         unique(seq(min(ySet), max(ySet), length=10))
      }
   }


   # ------ Create outer margin for title and labels ------------------------
   opar <- par(oma = par("oma") + c(2,2,4,2))  # Rahmen um Gesamtarray
   on.exit(par(opar))


   # ------ Create layout ---------------------------------------------------
   layoutMatrix <- matrix(rep(0, width * 2 * height),
                          2 * height,
                          width,
                          byrow = TRUE)
   widthArray <- rep(100, width)
   heightArray <- rep(100, 2 * height)
   for(i in seq(2, 2 * height, by=2)) {
      heightArray[i] <- max(20, (100 * height) / 12)
   }

   figureIndex <- 1
   for(y in 1:height) {
      for(x in 1:width) {
         if(figureIndex <= length(aLevels)) {
            layoutMatrix[2*(y - 1) + 2, x] <- figureIndex
            figureIndex <- figureIndex + 1
         }
      }
   }
   for(y in 1:height) {
      for(x in 1:width) {
         layoutMatrix[2*(y - 1) + 1, x] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }

   nf <- layout(layoutMatrix, widthArray, heightArray, TRUE)
   # layout.show(nf)

   # ------ Create a a value labels -----------------------------------------
   for(a in 1:length(aLevels)) {
      par(mar=c(1, 1, 1, 1))
      plot.new()
      plot.window(c(0, 1), c(0, 1))
      rect(0, 0, 1, 1, col=aLevelsColorArray[a])
      text(0.5, 0.5, aLevels[a])
   }


   # ------ Plot curves -----------------------------------------------------
   labelShrinkFactor <- 0.5   # 1.0 / max(length(aLevels), length(bLevels))
   legendShrinkFactor <- 0.75

   for(aPosition in seq(1, length(aLevels))) {
      a <- aLevels[aPosition]

      oldPar <- par(mar=par("mar") + c(3,3,3,3),
                     cex.lab=par("cex.lab") * labelShrinkFactor,
                     cex=par("cex") * legendShrinkFactor)

      xSubset <- subset(xSet, (aSet == a))
      ySubset <- subset(ySet, (aSet == a))
      zSubset <- subset(zSet, (aSet == a))
      vSubset <- subset(vSet, (aSet == a))
      wSubset <- subset(wSet, (aSet == a))

      subMainTitle <- ""
      xSubTitle <- xTitle
      ySubTitle <- yTitle
      plotstd3(subMainTitle,
               xSubTitle, ySubTitle, zTitle,
               xSubset, ySubset, zSubset,
               vSubset, wSubset, vTitle, wTitle,
               xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
               type = type,
               hideLegend = hideLegend,
               legendPos = legendPos,
               colorMode = colorMode,
               frameColor=frameColor)
      par(oldPar)
   }



   # ------ Create title and labels -----------------------------------------
   mtext(mainTitle, col=frameColor, side=3, at=0.5, line=0.5, outer=TRUE,
         xpd = NA, font = par("font.main"), cex = par("cex.main"))
   mtext(aTitle, col=frameColor, side=1, at=0.5, line=0.5, outer=TRUE,
         xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
}


# 2-dimensional array of plotstd3 plots
plotstd5 <- function(mainTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                     aSet, bSet, xSet, ySet, zSet,
                     vSet              = c(),
                     wSet              = c(),
                     vTitle            = "??vTitle??",
                     wTitle            = "??wTitle??",
                     aAxisTicks        = length(levels(factor(aSet))) > 1,
                     bAxisTicks        = length(levels(factor(bSet))) > 1,
                     xAxisTicks        = c(), yAxisTicks = c(),
                     confidence        = 0.95,
                     hbarSet           = c(),
                     hbarMeanSteps     = 10,
                     xSeparatorsSet    = c(),
                     xSeparatorsTitles = c(),
                     xSeparatorsColors  = c(),
                     type              = "lines",
                     hideLegend        = FALSE,
                     legendPos         = c(0,1),
                     colorMode         = cmColor,
                     frameColor        = par("fg"))
{
   aLevels <- levels(factor(aSet))
   bLevels <- levels(factor(bSet))
   if(length(aLevels) < 1) {
      cat("WARNING: plotstd5() - aLevels=c()\n")
      return(0);
   }
   if(length(bLevels) < 1) {
      cat("WARNING: plotstd5() - bLevels=c()\n")
      return(0);
   }

   if(colorMode == cmColor) {
      aLevelsColorArray <- rainbow(length(aLevels), start=0, end=3/6)
      bLevelsColorArray <- rainbow(length(bLevels), start=4/6, end=6/6)
   }
   else if(colorMode == cmGrayScale) {
      aLevelsColorArray <- graybow(length(aLevels))
      bLevelsColorArray <- graybow(length(bLevels))
   }
   else {
      aLevelsColorArray <- rep("#ffffff", length(aLevels))
      bLevelsColorArray <- rep("#ffffff", length(bLevels))
   }

   plotALabels <- 0
   plotBLabels <- 0
   if((aAxisTicks) || (length(levels(factor(aSet))) > 1)) {
      plotALabels <- 1
   }
   if((bAxisTicks) || (length(levels(factor(bSet))) > 1)) {
      plotBLabels <- 1
   }
   aLabel <- getVariable(aTitle)
   if(getAbbreviation(aTitle) != getVariable(xTitle)) {
      aLabel <- paste(sep="", aLabel, " ", getAbbreviation(aTitle), "")
   }
   if(getUnit(aTitle) != "") {
      aLabel <- paste(sep="", aLabel, " [", getUnit(aTitle), "]")
   }
   bLabel <- getVariable(bTitle)
   if(getAbbreviation(bTitle) != getVariable(bTitle)) {
      bLabel <- paste(sep="", bLabel, " ", getAbbreviation(bTitle), "")
   }
   if(getUnit(bTitle) != "") {
      bLabel <- paste(sep="", bLabel, " [", getUnit(bTitle), "]")
   }


   # ------ Get x/y tickmarks -----------------------------------------------
   if(missing(xAxisTicks) || (length(xAxisTicks) == 0)) {
      xAxisTicks = levels(factor(xSet))
      if(length(xAxisTicks) > 20) {
         unique(seq(min(xSet), max(xSet), length=10))
      }
   }
   if(missing(yAxisTicks) || (length(yAxisTicks) == 0)) {
      yAxisTicks = levels(factor(ySet))
      if(length(yAxisTicks) > 20) {
         unique(seq(min(ySet), max(ySet), length=10))
      }
   }


   # ------ Create outer margin for title and labels ------------------------
   opar <- par(oma = par("oma") + c(2,2,4,2))  # Rahmen um Gesamtarray
   on.exit(par(opar))


   # ------ Create layout ---------------------------------------------------
   layoutMatrix <- matrix(rep(0, (length(bLevels) + plotALabels) * (length(aLevels) + plotBLabels)),
                          length(bLevels) + plotALabels,
                          length(aLevels) + plotBLabels,
                          byrow = TRUE)
   widthArray <- rep(100, length(aLevels) + 1)
   if(plotBLabels > 0) {
      widthArray[1] <- max(30, (100 * length(aLevels)) / 12)
   }
   heightArray <- rep(100, length(bLevels) + 1)
   if(plotALabels > 0) {
      heightArray[length(bLevels) + 1] <- max(20, (100 * length(bLevels)) / 12)
   }

   figureIndex <- 1
   if(plotALabels > 0) {
      for(a in 1:length(aLevels)) {
         layoutMatrix[length(bLevels) + 1, a + plotBLabels] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }
   if(plotBLabels > 0) {
      for(b in 1:length(bLevels)) {
         layoutMatrix[1 + length(bLevels) - b, 1] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }
   for(b in 1:length(bLevels)) {
      for(a in 1:length(aLevels)) {
         layoutMatrix[1 + length(bLevels) - b, a + plotBLabels] <- figureIndex
         figureIndex <- figureIndex + 1
      }
   }

   nf <- layout(layoutMatrix, widthArray, heightArray, TRUE)
   # layout.show(nf)


   # ------ Create a and b value labels -------------------------------------
   if(plotALabels > 0) {
      for(a in 1:length(aLevels)) {
         par(mar=c(1, 1, 1, 1))
         plot.new()
         plot.window(c(0, 1), c(0, 1))
         rect(0, 0, 1, 1, col=aLevelsColorArray[a])
         text(0.5, 0.5, aLevels[a])
      }
   }

   if(plotBLabels > 0) {
      for(b in 1:length(bLevels)) {
         par(mar=c(1, 1, 1, 1))
         plot.new()
         plot.window(c(0, 1), c(0, 1))
         rect(0, 0, 1, 1, col=bLevelsColorArray[b])
         text(0.5, 0.5, bLevels[b], srt=90)
      }
   }


   # ------ Plot curves -----------------------------------------------------
   labelShrinkFactor <- 0.5   # 1.0 / max(length(aLevels), length(bLevels))
   legendShrinkFactor <- 0.75

   for(bPosition in seq(1, length(bLevels))) {
      b <- bLevels[bPosition]
      for(aPosition in seq(1, length(aLevels))) {
         a <- aLevels[aPosition]

         oldPar <- par(mar=par("mar") + c(3,3,3,3),
                       cex.lab=par("cex.lab") * labelShrinkFactor,
                       cex=par("cex") * legendShrinkFactor)

         xSubset <- subset(xSet, (aSet == a) & (bSet == b))
         ySubset <- subset(ySet, (aSet == a) & (bSet == b))
         zSubset <- subset(zSet, (aSet == a) & (bSet == b))
         vSubset <- subset(vSet, (aSet == a) & (bSet == b))
         wSubset <- subset(wSet, (aSet == a) & (bSet == b))

         subMainTitle <- ""
         xSubTitle <- xTitle
         ySubTitle <- yTitle
         plotstd3(subMainTitle,
                  xSubTitle, ySubTitle, zTitle,
                  xSubset, ySubset, zSubset,
                  vSubset, wSubset, vTitle, wTitle,
                  xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                  confidence = confidence,
                  hbarSet = hbarSet,
                  hbarMeanSteps = hbarMeanSteps,
                  xSeparatorsSet = xSeparatorsSet,
                  xSeparatorsTitles = xSeparatorsTitles,
                  xSeparatorsColors = xSeparatorsColors,
                  type = type,
                  hideLegend=hideLegend,
                  legendPos=legendPos,
                  colorMode = colorMode,
                  frameColor=frameColor)
         par(oldPar)
      }
   }


   # ------ Create title and labels -----------------------------------------
   mtext(mainTitle, col=frameColor, side=3, at=0.5, line=0.5, outer=TRUE,
         xpd = NA, font = par("font.main"), cex = par("cex.main"))
   if(plotALabels > 0) {
      mtext(aLabel, col=frameColor,
            side=1, at=0.5, line=0.5, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }
   if(plotBLabels > 0) {
      mtext(bLabel, col=frameColor,
            side=2, at=0.5, line=0.5, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
   }
}


# multi-page 2-dimensional array of plotstd3 plots
plotstd6 <- function(mainTitle, pTitle, aTitle, bTitle, xTitle, yTitle, zTitle,
                     pSet, aSet, bSet, xSet, ySet, zSet,
                     vSet              = c(),
                     wSet              = c(),
                     vTitle            = "??vTitle??",
                     wTitle            = "??wTitle??",
                     aAxisTicks        = length(levels(factor(aSet))) > 1,
                     bAxisTicks        = length(levels(factor(bSet))) > 1,
                     xAxisTicks        = c(),
                     yAxisTicks        = c(),
                     confidence        = 0.95,
                     hbarSet           = c(),
                     hbarMeanSteps     = 10,
                     xSeparatorsSet    = c(),
                     xSeparatorsTitles = c(),
                     xSeparatorsColors  = c(),
                     type              = "lines",
                     pStart            = 0,
                     hideLegend        = FALSE,
                     legendPos         = c(0,1),
                     colorMode         = FALSE,
                     frameColor        = par("fg"),
                     simulationName    = "")
{
   if(length(pSet) == 0) {
      pSet <- rep(1, length(xSet))
   }
   if(length(aSet) == 0) {
      aSet <- rep(0, length(xSet))
   }
   if(length(bSet) == 0) {
      bSet <- rep(0, length(xSet))
   }
   pLevels <- levels(factor(pSet))
   index <- 0
   for(p in pLevels) {
      oldPar1 <- par(mar=c(3,3,3,3), oma=c(2,2,4,2), bg=getBackgroundColor(index, colorMode, pStart))

      xSubset <- subset(xSet, (pSet == p))
      ySubset <- subset(ySet, (pSet == p))
      zSubset <- subset(zSet, (pSet == p))
      vSubset <- subset(vSet, (pSet == p))
      wSubset <- subset(wSet, (pSet == p))
      aSubset <- subset(aSet, (pSet == p))
      bSubset <- subset(bSet, (pSet == p))
      if(pTitle != "") {
         subMainTitle <- paste(sep="", pTitle, " = ", p)
      }
      else {
         subMainTitle <- ""
      }
      if(length(pLevels) <= 1) {
         subMainTitle <- ""
      }

      if(length(levels(factor(bSet))) <= 1) {
         if(length(levels(factor(aSet))) <= 1) {
            plotstd3(subMainTitle,
                     xTitle, yTitle, zTitle,
                     xSubset, ySubset, zSubset,
                     vSubset, wSubset, vTitle, wTitle,
                     xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                     confidence = confidence,
                     hbarSet = hbarSet,
                     hbarMeanSteps = hbarMeanSteps,
                     xSeparatorsSet = xSeparatorsSet,
                     xSeparatorsTitles = xSeparatorsTitles,
                     xSeparatorsColors = xSeparatorsColors,
                     type = type,
                     hideLegend = hideLegend,
                     legendPos = legendPos,
                     colorMode = colorMode,
                     frameColor = frameColor)
         }
         else {
            oldPar2 <- par(font.main=3, cex.main=0.9)
            plotstd4(subMainTitle,
                     aTitle, xTitle, yTitle, zTitle,
                     aSubset, xSubset, ySubset, zSubset,
                     vSubset, wSubset, vTitle, wTitle,
                     aAxisTicks = aAxisTicks,
                     xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                     confidence = confidence,
                     hbarSet = hbarSet,
                     hbarMeanSteps = hbarMeanSteps,
                     xSeparatorsSet = xSeparatorsSet,
                     xSeparatorsTitles = xSeparatorsTitles,
                     xSeparatorsColors = xSeparatorsColors,
                     type = type,
                     hideLegend = hideLegend,
                     legendPos = legendPos,
                     colorMode = colorMode,
                     frameColor = frameColor)
            par(oldPar2)
         }
      }
      else {
         oldPar2 <- par(font.main=3, cex.main=0.9)
         plotstd5(subMainTitle,
                  aTitle, bTitle, xTitle, yTitle, zTitle,
                  aSubset, bSubset, xSubset, ySubset, zSubset,
                  vSubset, wSubset, vTitle, wTitle,
                  aAxisTicks = aAxisTicks, bAxisTicks = bAxisTicks,
                  xAxisTicks = xAxisTicks, yAxisTicks = yAxisTicks,
                  confidence = confidence,
                  hbarSet = hbarSet,
                  hbarMeanSteps = hbarMeanSteps,
                  xSeparatorsSet = xSeparatorsSet,
                  xSeparatorsTitles = xSeparatorsTitles,
                  xSeparatorsColors = xSeparatorsColors,
                  type = type,
                  hideLegend = hideLegend,
                  legendPos = legendPos,
                  colorMode = colorMode,
                  frameColor = frameColor)
         par(oldPar2)
      }

      mtext(mainTitle, col=frameColor,
            side=3, at=0.5, line=0.5, outer=TRUE,
            xpd = NA, font = par("font.main"), cex = par("cex.main"))
      mtext(simulationName,
            side=1, at=0, line=0.5, adj=0, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))
      mtext(paste(sep="", "Copyright © ", format(Sys.time(), "%Y"), " Thomas Dreibholz - ", date()),
            side=1, at=1, line=0.5, adj=1, outer=TRUE,
            xpd = NA, font = par("font.lab"), cex = par("cex.lab"))

      par(oldPar1)
      index <- index + 1
   }
}


# Get "useful" ticks from given data set
getUsefulTicks <- function(set, count = 10, integerOnly = FALSE)
{
   if(length(set) < 1) {
      stop("getUsefulTicks: The set is empty!")
   }
   minValue <- min(set)
   maxValue <- max(set)
   if(minValue == maxValue) {
      minValue <- floor(minValue)
      maxValue <- ceiling(minValue)
      if(minValue == maxValue) {
         minValue <- minValue - 1
         maxValue <- maxValue + 1
      }
      set <- c(minValue, maxValue)
   }
   divSet <- c(1, 0.5, 0.25, 0.10)
   if(integerOnly) {
      minValue <- floor(minValue)
      maxValue <- ceiling(maxValue)
      divSet <- c(1)
   }
   difference <- maxValue - minValue
   stepOrder <- floor(log(difference, base=10))

   for(x in divSet) {
      stepBase <- x * (10^stepOrder)

      minFactor <- floor(minValue / stepBase)
      maxFactor <- ceiling(maxValue / stepBase)
      minStep <- minFactor * stepBase
      maxStep <- maxFactor * stepBase

      ticks <- seq(minStep, maxStep, by=stepBase)
      if(length(ticks) >= (count / 2)) {
         return(ticks)
      }
   }
   return(ticks)
}


# Get "useful" integer ticks from given data set
getIntegerTicks <- function(set, count = 10)
{
   if(length(set) < 1) {
      stop("getIntegerTicks: The set is empty!")
   }
   return(getUsefulTicks(set, count, integerOnly = TRUE))
}


# Read table from results file
loadResults <- function(name, customFilter="")
{
   filter <- "cat"
   if(any(grep(".bz2", name))) {
      filter <- "bzcat"
   }
   else if(any(grep(".gz", name))) {
      filter <- "zcat"
   }
   filter <- paste(sep="", filter, " ", name)
   if(customFilter != "") {
      filter <- paste(sep="", filter, " | ", customFilter)
   }

   dataInputCommand <- filter
   cat(sep="","Loading from pipe [", dataInputCommand, "]...\n")
   data <- read.table(pipe(dataInputCommand))
   return(data)
}
