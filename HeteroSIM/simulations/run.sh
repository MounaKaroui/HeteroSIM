cmdPrefix="opp_run -m -u Cmdenv -c confName" # TODO configure correct confName (e.g. delayWeighted)
cmdSuffix="-n ..:../../lib/inet/src:../../lib/inet/examples:../../lib/inet/tutorials:../../lib/inet/showcases:../../lib/simulte/simulations:../../lib/simulte/src --image-path=../../lib/inet/images:../../lib/simulte/images -l ../src/HeteroSIM -l ../../lib/inet/src/INET -l ../../lib/simulte/src/lte examples/unicast/ModelEvaluationScenario.ini"

totalRun=2 # TODO configure the correct total run numbers from e.g. output of cmd 'opp_run -c confName -q numruns ModelEvaluationScenario.ini' 

for i in `seq $totalRun`
do
	echo "##### RUNNING SIMULATION  $i / $totalRun ...."
	runID=$(expr $i - 1)
	eval "$cmdPrefix -r $runID $cmdSuffix"
done

