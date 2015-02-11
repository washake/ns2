set val(ifqlen)		50
set val(nn)		3
set val(rp)		FTFP
set val(chan)		Channel/WirelessChannel
set val(prop)		Propagation/TwoRayGround
set val(netif)		Phy/WirelessPhy
set val(mac)		Mac/802_11
set val(ifq)		Queue/DropTail/PriQueue
set val(ll)		LL 
set val(ant)		Antenna/OmniAntenna
set val(x)		1000
set val(y)		500
set val(stop_)		550


set ns 		[new Simulator]
set tracefd 	[open ftfp.tr w]
set namtrace 	[open ftfp.nam w]

$ns use-newtrace
$ns trace-all $tracefd
$ns namtrace-all-wireless $namtrace $val(x) $val(y)

#set up topography
set topo 	[new Topography]
$topo load_flatgrid $val(x) $val(y)

create-god $val(nn)


set channel [new Channel/WirelessChannel]
$channel set errorProbability_ 0.0

$ns node-config -adhocRouting $val(rp) \
		-llType $val(ll) \
		-macType $val(mac) \
		-ifqType $val(ifq) \
		-ifqLen	$val(ifqlen) \
		-antType $val(ant) \
		-propType $val(prop) \
		-phyType $val(netif) \
		-channel $channel \
		-topoInstance $topo \
		-agentTrace ON \
		-routerTrace ON \
		-macTrace OFF
		
for {set i 0} {$i < $val(nn)} {incr i} {
   set node_($i) [$ns node]
   $node_($i) random-motion 0
   }
   
$node_(0) set X_ 50.0
$node_(0) set Y_ 200.0
$node_(0) set Z_ 0.0

$node_(1) set X_ 250.0
$node_(1) set Y_ 200.0
$node_(1) set Z_ 0.0

$node_(2) set X_ 500.0
$node_(2) set Y_ 200.0
$node_(2) set Z_ 0.0


proc stop {} {
  global ns tracefd namtrace
  $ns flush-trace
  close $tracefd
  close $namtrace
  
  exec nam ftfp.nam & 
  exit 0
}

for {set i 0} {$i < $val(nn)} {incr i} {
  $ns initial_node_pos $node_($i) 20
   }
   
set udp_(0) [new Agent/UDP]
$ns attach-agent $node_(0) $udp_(0)

set null_(0) [new Agent/Null]
$ns attach-agent $node_(2) $null_(0)

$ns connect $udp_(0) $null_(0)

set cbr_(0) [new Application/Traffic/CBR]
$cbr_(0) set packetSize_ 512
$cbr_(0) set interval_ 0.1
$cbr_(0) set random_ 1
$cbr_(0) set maxpkts_ 10000
$cbr_(0) attach-agent $udp_(0)

$ns at 10.0 "$cbr_(0) start"

for {set i 0} {$i < $val(nn)} {incr i} {
  $ns at $val(stop_).0 "$node_($i) reset";
   }
   
$ns at $val(stop_).0 "stop"
$ns at $val(stop_).01 "puts \"NS EXITING...\" ; $ns halt"
puts "Starting Simulation"
$ns run