<?php
use go\Chan;
use go\Scheduler;

$ch = new Chan(1);
go(function() use($ch){
    echo "Alice sends a greeting\n";
    $ch->push("Greeting from Alice\n");
});

$message = $ch->pop();
echo $message;
