# ###########################################################################
# Name:        plot-paper1
# Description: Paper Plots
# Revision:    $Id$
# ###########################################################################

source("attack-plottertemplates.R")


# ------ Plotter Settings ---------------------------------------------------
plotColorMode         <- cmColor
plotHideLegend        <- FALSE
plotLegendSizeFactor  <- 0.8
plotOwnOutput         <- TRUE
plotFontFamily        <- "Helvetica"
plotFontPointsize     <- 22
plotWidth             <- 10
plotHeight            <- 10
plotConfidence        <- 0.95

# ###########################################################################

# ------ Plots --------------------------------------------------------------
plotConfigurations <- list(
   list("attack-registration-planetlabI", "attack-registration-planetlabI-HandlingSpeed.pdf",
        "Pool Element Attack (PlanetLab)", NA, list(seq(0,85,10)), list(1,0),
        "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", ""),
   list("attack-registration-countermeasure-planetlabII", "attack-registration-countermeasure-planetlabII-HandlingSpeed.pdf",
        "PlanetLab", list(seq(0,10,1)),  list(seq(0,85,10)), list(1,0),
        "Attackers", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackInterval"),

   list("attack-handleResolution-planetlabI", "attack-handleResolution-planetlabI-HandlingSpeed.pdf",
        "Pool User Attack (PlanetLab)", NA,  list(seq(0,85,10)), list(1,0),
        "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability"),
   list("attack-handleResolution-countermeasure-planetlabI", "attack-handleResolution-countermeasure-planetlabI-HandlingSpeed.pdf",
        "PlanetLab", NA,  list(seq(0,85,10)), list(1,0),
        "AttackInterval", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability"),
   list("attack-handleResolution-countermeasure-planetlabII", "attack-handleResolution-countermeasure-planetlabII-HandlingSpeed.pdf",
        "PlanetLab", list(seq(0,10,1)),  list(seq(0,85,10)), list(1,0),
        "Attackers", "calcAppPoolUser-CalcAppPUAverageHandlingSpeedPercent",
        "Policy", "AttackReportUnreachableProbability", "AttackInterval")
)

# ###########################################################################

createPlots(measurementDirectory, plotConfigurations)
