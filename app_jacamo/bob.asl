// Agent: bob

/* Initial beliefs and rules */

/* Initial goals */
!start.

/* Plans */
+!start: true
	<- .print("Bob is running").

/* Communication: request(Target, Operation, Data) */
+ns1::request(Target, Operation, Data): Operation == setLed | Operation == setRelay | Operation == setMotor
	<- .print("Request => target: ", Target, ", operation: ", Operation, " = ", Data);
	/* Acting: action(Thing, Action, Input) */
	ns1::acting(action(Target, Operation, Data));
	.print("Action: ", Operation, " ", Data).

+ns1::request(Target, Operation, Data): Target \== "unknown" & Operation \== "unknown"
	<- .print("Message => target - ", Target, ", operation: ", Operation, " = ", Data).

/* Reporting: report(Thing, Property, Value) */
+ns1::report(Thing, Property, Value): Property == ledState
	<- -+ledState(Thing, Value);
	.print("Device => ", Thing," - property: ", Property, " = ", Value).

+ns1::report(Thing, Property, Value): Property == relayState
	<- -+relayState(Thing, Value);
	.print("Device => ", Thing," - property: ", Property, " = ", Value).

+ns1::report(Thing, Property, Value): Property == motorState
	<- -+motorState(Thing, Value);
	.print("Device => ", Thing," - property: ", Property, " = ", Value).

+ns1::report(Thing, Property, Value): Property == temperature
	<- -+temperature(Thing, Value);
	.print("Device => ", Thing," - property: ", Property, " = ", Value).

+ns1::report(Thing, Property, Value): Property == humidity
	<- -+humidity(Thing, Value);
	.print("Device => ", Thing," - property: ", Property, " = ", Value).

+ns1::report(Thing, Property, Value): Property == illuminance
	<- -+illuminance(Thing, Value);
	.print("Device => ", Thing," - property: ", Property, " = ", Value).

+ns1::report(Thing, Property, Value): Thing \== "unknown" & Property == "unknown"
	<- .print("Device => ", Thing," - property: ", Property, " = ", Value).

/* Monitoring: event(Thing, Event, Data) */
+ns1::event(Thing, Event, Data)
	<- -+event(Thing, Event, Data);
	.print("Alert => device: ", Thing," - event: ", Event, " = ", Data).

{ include("$jacamoJar/templates/common-cartago.asl") }
// { include("$jacamoJar/templates/common-moise.asl") }
// { include("$jacamoJar/templates/org-obedient.asl") }
