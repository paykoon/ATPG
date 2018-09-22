#!/bin/sh
#the script is used to read all blif files and test patterns to do the simulation.

#read all blif file
blifFolder=`ls blifFile`
for blifFile in $blifFolder
do
  screen_name=${blifFile/.blif/}ATPGTSATESTGENERATION
  testFile=${blifFile/.blif/.patterns}
  resultFile=${blifFile/.blif/.result}
  screen -dmS $screen_name
  #Only considers the faults among our fault model
  #cmd=$"./ATPG_faultModel ./blifFile/${blifFile} ./SSAFpatterns/${testFile}  >> ./result/onlyUseFaultModel/${resultFile}"
  #test all faults.
  cmd=$"time ./ATPGTSATESTGENERATION ./blifFile/${blifFile} ./SSAFpatterns/${testFile}  >> ./result/0921ITCCircuit/${resultFile}"
  #test generate all additional DSA test patterns.
  #cmd=$"./ATPGCheckAllTSA2  ./blifFile/${blifFile} ./SSAFpatterns/${testFile}  >> ./result/0728ATPGCheckAllTSA/${resultFile}"

  screen -x -S $screen_name -p 0 -X stuff "$cmd"
  screen -x -S $screen_name -p 0 -X stuff $'\n'
done
