#!/bin/sh
#the script used to read all blif files and test patterns to do the simulation.

#read all blif file
blifFolder=`ls blifFile`
for blifFile in $blifFolder
do
  screen_name=${blifFile/.blif/}
  testFile=${blifFile/.blif/.patterns}
  resultFile=${blifFile/.blif/.result}
  screen -dmS $screen_name
  #Only considers the faults among our fault model
  #cmd=$"./ATPG_faultModel ./blifFile/${blifFile} ./SSAFpatterns/${testFile}  >> ./result/onlyUseFaultModel/${resultFile}"
  #test all faults.
  #cmd=$"time ./ATPG ./blifFile/${blifFile} ./SSAFpatterns/${testFile}  >> ./result/allFault/${resultFile}"
  #test generate all additional DSA test patterns.
  cmd=$"./ATPG ./blifFile/${blifFile} ./SSAFpatterns/${testFile}  >> ./result/CheckTestCoverageAgain0712/${resultFile}"

  screen -x -S $screen_name -p 0 -X stuff "$cmd"
  screen -x -S $screen_name -p 0 -X stuff $'\n'
done
