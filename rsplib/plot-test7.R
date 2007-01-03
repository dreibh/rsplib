source("plotter.R")


data <- loadResults("test.data")

xSet <- data$Value1
zSet <- data$Type


frameColor <- "gold4"


pdf("test7.pdf", width=10, height=10, onefile=TRUE, family="Helvetica", pointsize=18)

# r <- hist(data$Value1, col="gray", border=frameColor, freq=TRUE,
#           xlab="X Label", ylab="Y Label")
# grid(20,20,lty=1)
# box(col=frameColor)
# r <- hist(data$Value1, col="gray", border=frameColor, freq=TRUE,
#           xlab="X Label", ylab="Y Label")




breakSet <- getUsefulTicks(xSet)


zLevels <- levels(factor(zSet))
count <- 0
for(zValue in zLevels) {
   xSubset <- subset(xSet, (zSet == zValue))
   cat("Z=",zValue,"\n")

   r <- hist(xSubset, br=breakSet, plot=FALSE)
   print(r)

   if(count == 0) {
      plot(r, col="gray")
      grid(20,20,lty=1)
      box(col=frameColor)
      lines(r, col="gray")
   }
   else {
      lines(r, col="blue")
   }

   count <- count + 1
}


dev.off()
