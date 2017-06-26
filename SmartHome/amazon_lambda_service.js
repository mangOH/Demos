'use strict';

/**
 * This demo implements handling for an Amazon Alexa skill which queries device data published by a
 * mangOH Red to AirVantage.
 */

// Global variables
var lat, long, address, APN, operator, RAT, roaming, modemModel, FWVersion;

// DF variables
var systemUid;

function initParams()
{
    console.log(`In initParams`);
    lat        = '';
    long       = '';
    APN        = '';
    operator   = '';
    RAT        = '';
    roaming    = '';
    modemModel = '';
    FWVersion  = '';
    systemUid  = '';
}

function logParams()
{
    console.log(`In logParams`);
    console.log("Modem    : " + modemModel);
    console.log("FW       : " + FWVersion);
    console.log("Operator : " + operator);
    console.log("APN      : " + APN);
    console.log("RAT      : " + RAT);
    console.log("Roaming  : " + roaming);
    console.log("LAT      : " + lat);
    console.log("LONG     : " + long);
}

// --------------- Helpers that build all of the responses -----------------------

function buildSpeechletResponse(title, output, repromptText, shouldEndSession) {
    return {
        outputSpeech: {
            type: 'PlainText',
            text: output,
        },
        card: {
            type: 'Simple',
            title: `SessionSpeechlet - ${title}`,
            content: `SessionSpeechlet - ${output}`,
        },
        reprompt: {
            outputSpeech: {
                type: 'PlainText',
                text: repromptText,
            },
        },
        shouldEndSession,
    };
}

function buildResponse(sessionAttributes, speechletResponse) {
    return {
        version: '1.0',
        sessionAttributes,
        response: speechletResponse,
    };
}

function buildLinkAccountResponse() {
    return {
        version: '1.0',
        response: {
            outputSpeech: {
                type:'PlainText',
                text:'Authentication is needed. Please go to your Alexa phone app and log into your account to proceed.'
            },
            card: {
                type: 'LinkAccount'
            },
        },
    };
}

function requestSensorData(accessToken, resultFunction) {
    var url = `https://eu.airvantage.net/api/v1/systems/data/raw?access_token=${accessToken}&targetIds=${systemUid}&dataIds=Sensors%2fAccelerometer%2fAcceleration%2fX,Sensors%2fAccelerometer%2fAcceleration%2fY,Sensors%2fAccelerometer%2fAcceleration%2fZ,Sensors%2fAccelerometer%2fGyro%2fX,Sensors%2fAccelerometer%2fGyro%2fY,Sensors%2fAccelerometer%2fGyro%2fZ,Sensors%2fPressure%2fPressure,Sensors%2fPressure%2fTemperature&size=1`;
    var request = require('request');
    request(
        url,
        function(error, response, body) {
            if (!error && response.statusCode === 200)
            {
                let j = JSON.parse(body);
                let sys = j[systemUid];
                // TODO: handle SyntaxError exception
                var readings = {
                    "acceleration": {
                        "x": sys["Sensors/Accelerometer/Acceleration/X"][0].v,
                        "y": sys["Sensors/Accelerometer/Acceleration/Y"][0].v,
                        "z": sys["Sensors/Accelerometer/Acceleration/Z"][0].v
                    },
                    "gyro": {
                        "x": sys["Sensors/Accelerometer/Gyro/X"][0].v,
                        "y": sys["Sensors/Accelerometer/Gyro/Y"][0].v,
                        "z": sys["Sensors/Accelerometer/Gyro/Z"][0].v
                    },
                    "temperature": sys["Sensors/Pressure/Temperature"][0].v,
                    "pressure": sys["Sensors/Pressure/Pressure"][0].v,
                };
                resultFunction(readings);
            } else {
                resultFunction(null);
            }
        });
}


// --------------- Functions that control the skill's behavior -----------------------

function sayHelloResponse(callback) {
    console.log(`In sayHelloResponse`);
    // If we wanted to initialize the session to have some attributes we could add those here.
    const sessionAttributes = {};
    const cardTitle = 'Welcome';
    const speechOutput =
        'Sierra Wireless Alexa mangOH Red demo. Please identify your modem using find.';
    // If the user either does not reply to the welcome message or says something that is not
    // understood, they will be prompted again with this text.
    const repromptText = 'Please identify your modem using find.';
    const shouldEndSession = false;

    callback(
        sessionAttributes,
        buildSpeechletResponse(cardTitle, speechOutput, repromptText, shouldEndSession));
}

function handleLocateIntent(intent, session, callback) {
    console.log(`In handleLocateIntent`);
    let shouldEndSession = false;
    let speechOutput = '';
    const sessionAttributes = {};

    var request = require('request');
    const getAddressURL = `https://maps.googleapis.com/maps/api/geocode/json?latlng=${lat},${long}`;
    request(
        getAddressURL,
        function (error, response, body) {
            if (!error && response.statusCode === 200) {
                var myObj = JSON.parse (body);
                var resultsArray = myObj.results;
                address = resultsArray[0].formatted_address;
                console.log("ADDRESS  : " + address);
                speechOutput = "Modem is located at latitude ";
                speechOutput = speechOutput + lat;
                speechOutput = speechOutput + ", longitude " + long;
                speechOutput = speechOutput + ". Address is " + address;
                speechOutput = speechOutput + ".";
                console.log (speechOutput);
                callback(sessionAttributes,
                     buildSpeechletResponse(intent.name, speechOutput, null, shouldEndSession));
            }
            else
            {
                console.log('GET address error');
            }
        });
}

function handleNetworkIntent (intent, session, callback) {
    console.log(`In handleNetworkIntent`);
    let shouldEndSession = false;
    const sessionAttributes = {};
    let speechOutput = "Modem is registered on " + operator;
    speechOutput = speechOutput + ". Technology is " + RAT;
    if ('No roaming' === roaming)
    {
        speechOutput = speechOutput + '. Modem is in home network.';
    }
    else
    {
        speechOutput = speechOutput + '. Modem is roaming.';
    }
    console.log (speechOutput);
    callback(sessionAttributes,
         buildSpeechletResponse(intent.name, speechOutput, null, shouldEndSession));
}

function handleDetailsIntent (intent, session, callback) {
    console.log(`In handleDetailsIntent`);
    let shouldEndSession = false;
    const sessionAttributes = {};
    let speechOutput = `Modem model is ${modemModel}, with firmware ${FWVersion}.`;
    console.log (speechOutput);
    callback(sessionAttributes,
         buildSpeechletResponse(intent.name, speechOutput, null, shouldEndSession));
}


function handleOrientationIntent(intent, session, callback)
{
    console.log(`in handleOrientationIntent`);
    
    function getOrientationString(acceleration) {
        function approxEq(v1, v2) {
            return Math.abs(v1 - v2) < 2.0;
        }

        console.log(
            `Getting orientation for acceleration (${acceleration.x}, ${acceleration.y}, ${acceleration.z})`);
        if (approxEq(acceleration.x, 9.8) && approxEq(acceleration.y, 0.0) &&
            approxEq(acceleration.z, 0.0)) {
            return "sitting on battery connector edge";
        } else if (approxEq(acceleration.x, -9.8) && approxEq(acceleration.y, 0.0) &&
                   approxEq(acceleration.z, 0.0)) {
            return "sitting on sim card edge";
        } else if (approxEq(acceleration.x, 0.0) && approxEq(acceleration.y, 9.8) &&
                   approxEq(acceleration.z, 0.0)) {
            return "sitting on downstream USB port edge";
        } else if (approxEq(acceleration.x, 0.0) && approxEq(acceleration.y, -9.8) &&
                   approxEq(acceleration.z, 0.0)) {
            return "sitting on raspberry pi connector edge";
        } else if (approxEq(acceleration.x, 0.0) && approxEq(acceleration.y, 0.0) &&
                   approxEq(acceleration.z, 9.8)) {
            return "upright";
        } else if (approxEq(acceleration.x, 0.0) && approxEq(acceleration.y, 0.0) &&
                   approxEq(acceleration.z, -9.8)) {
            return "upside down";
        }
        return "unknown";
        //return `unknown with x = ${acceleration.x}, y = ${acceleration.y} and z  ${acceleration.z} `;
    }

    function speak(text)
    {
        console.log(text);
        const sessionAttributes = {};
        const shouldEndSession = false;
        callback(sessionAttributes, buildSpeechletResponse(intent.name, text, null, shouldEndSession));
    }

    function sensorDataCallback(sensorData) {
        if (sensorData == null) {
            speak("Failed to acquire sensor data.");
        } else {
            speak('The orientation is ' + getOrientationString(sensorData.acceleration));
        }
    }

    if (systemUid == undefined)
    {
        speak("No modem has been selected.  Use the find command");
    }
    else
    {
        requestSensorData(session.user.accessToken, sensorDataCallback);
    }
}


function handleAccelerationIntent(intent, session, callback)
{
    function speak(text)
    {
        console.log(text);
        const sessionAttributes = {};
        const shouldEndSession = false;
        callback(
            sessionAttributes,
            buildSpeechletResponse(intent.name, text, null, shouldEndSession));
    }
    
    function sensorDataCallback(sensorData) {
        if (sensorData == null) {
            speak("Failed to acquire sensor data.");
        } else {
            const acc = Math.sqrt(
                Math.pow(sensorData.acceleration.x, 2) +
                Math.pow(sensorData.acceleration.y, 2) +
                Math.pow(sensorData.acceleration.z, 2));
            const accRounded = Math.round(acc * 100) / 100.0;
            speak('The acceleration is ' + accRounded + ' meters per second squared');
        }
    }
    
    if (systemUid == undefined)
    {
        speak("No modem has been selected.  Use the find command");
    }
    else
    {
        requestSensorData(session.user.accessToken, sensorDataCallback);
    }
}

function handleTemperatureIntent(intent, session, callback) {
    function speak(text)
    {
        console.log(text);
        const sessionAttributes = {};
        const shouldEndSession = false;
        callback(
            sessionAttributes,
            buildSpeechletResponse(intent.name, text, null, shouldEndSession));
    }
    
    function sensorDataCallback(sensorData) {
        if (sensorData == null) {
            speak("Failed to acquire sensor data.");
        } else {
            speak('The board temperature is ' + sensorData.temperature + ' degrees celcius');
        }
    }
    
    if (systemUid == undefined)
    {
        speak("No modem has been selected.  Use the find command");
    }
    else
    {
        requestSensorData(session.user.accessToken, sensorDataCallback);
    }
}


function handlePressureIntent(intent, session, callback) {
    function speak(text)
    {
        console.log(text);
        const sessionAttributes = {};
        const shouldEndSession = false;
        callback(
            sessionAttributes,
            buildSpeechletResponse(intent.name, text, null, shouldEndSession));
    }
    
    function sensorDataCallback(sensorData) {
        if (sensorData == null) {
            speak("Failed to acquire sensor data.");
        } else {
            const pressureRounded = Math.round(sensorData.pressure * 100) / 100.0;
            speak('The air pressure is ' + pressureRounded + ' kilopascals');
        }
    }
    
    if (systemUid == undefined)
    {
        speak("No modem has been selected.  Use the find command");
    }
    else
    {
        requestSensorData(session.user.accessToken, sensorDataCallback);
    }
}


function handleFindIntent (intent, session, callback) {
    console.log(`In handleFindIntent`);
    initParams();
    var name_of_modem;
    var slotName;
    var repromptText = 'Please tell me what you want to do';
    const sessionAttributes = {};
    let shouldEndSession = false;
    let speechOutput = '';
    var messageSlot = intent.slots.name;
    var str = '';

    if (messageSlot) {
        slotName = messageSlot.name;
        name_of_modem = messageSlot.value;
    }

    if(name_of_modem)
    {
        var http_path = 'https://eu.airvantage.net/api/v1/systems?fields=data,uid,applications&name='
                        + name_of_modem
                        + '&access_token='
                        + session.user.accessToken;
        console.log(http_path);
        var request = require('request');
        request(http_path, function (error, response, body)
        {
            console.log("inside request callback")
            if (!error && response.statusCode === 200)
            {
                var myObj = JSON.parse (body);
                var items;
                if (myObj.size>0)
                {
                    items =  myObj.items;
                    if (items.length === 1) // We have 1 item
                    {
                        systemUid = items[0].uid;
                        console.log(`Just set systemUid to ${systemUid} and items[0].uid == ${items[0].uid}`);
                        var dataArray =  items[0].data;
                        lat      = dataArray.latitude;
                        long     = dataArray.longitude;
                        APN      = dataArray.apn;
                        operator = dataArray.networkOperator;
                        RAT      = dataArray.networkServiceType;
                        roaming  = dataArray.roamingStatus;

                        var applicationsArray = items[0].applications;

                        for (var index=0;index<applicationsArray.length;index++) {
                            if (applicationsArray[index].category === "FIRMWARE") {
                                modemModel = applicationsArray[index].type;
                                FWVersion  = applicationsArray[index].revision;
                            }
                        }
                        //console.log(body);
                        speechOutput = `OK. Modem found.`;
                        console.log(speechOutput);
                        callback(sessionAttributes,
                                buildSpeechletResponse(intent.name, speechOutput,
                                repromptText, shouldEndSession));
                    }
                    else
                    {
                        speechOutput = `${items.length} modems found.'
                                       + 'Please use a more specific name to uniquely identify your modem.`;
                        repromptText = 'Please identify your modem using find.';
                        console.log(speechOutput);
                        callback(sessionAttributes,
                                buildSpeechletResponse(intent.name, speechOutput,
                                repromptText, shouldEndSession));
                    }
                }
                else // We do not have any items
                {
                    speechOutput = `Modem not found. Please identify your modem using find.`;
                    repromptText = 'Please identify your modem using find.';
                    console.log(speechOutput);
                    callback(sessionAttributes,
                            buildSpeechletResponse(intent.name, speechOutput,
                            repromptText, shouldEndSession));
                }

            }
            else
            {
                console.log ("GET AVMS Error");
                speechOutput = `Error is AVMS get. Ending session.`;
                shouldEndSession = true;
                callback(sessionAttributes,
                        buildSpeechletResponse(intent.name, speechOutput,
                        null, shouldEndSession));
            }
        });
    }
    else
    {
        speechOutput = `I do not understand the name of the modem.
                        Please identify your modem using find.`;
        repromptText = 'Please identify your modem using find.';
        console.log(speechOutput);
        callback(sessionAttributes,
                buildSpeechletResponse(intent.name, speechOutput,
                repromptText, shouldEndSession));
    }
    console.log('Exiting handleFindIntent');
}

function handleSessionEndRequest(callback) {
    const cardTitle = 'Session Ended';
    const speechOutput = 'Thank you for using the mangOH Red Alexa demo. Have a nice day!';
    // Setting this to true ends the session and exits the skill.
    const shouldEndSession = true;

    callback({}, buildSpeechletResponse(cardTitle, speechOutput, null, shouldEndSession));
}


// --------------- Events -----------------------

/**
 * Called when the session starts.
 */
function onSessionStarted(sessionStartedRequest, session) {
    console.log(`onSessionStarted requestId=${sessionStartedRequest.requestId}, sessionId=${session.sessionId}`);
}

/**
 * Called when the user launches the skill without specifying what they want.
 */
function onLaunch(launchRequest, session, callback) {
    console.log(`onLaunch requestId=${launchRequest.requestId}, sessionId=${session.sessionId}`);
    sayHelloResponse(callback);
}

/**
 * Called when the user specifies an intent for this skill.
 */
function onIntent(intentRequest, session, callback) {
    console.log(`onIntent requestId=${intentRequest.requestId}, sessionId=${session.sessionId}`);

    const intent = intentRequest.intent;
    const intentName = intentRequest.intent.name;

    console.log(`onIntent intent=${intent}, intentName=${intentName}`);

    // Dispatch to your skill's intent handlers
    if (intentName === 'find') {
        handleFindIntent(intent, session, callback);
    } else if (intentName === 'details') {
        handleDetailsIntent(intent, session, callback);
    } else if (intentName === 'locate') {
        handleLocateIntent(intent, session, callback);
    } else if (intentName === 'network') {
        handleNetworkIntent(intent, session, callback);
    } else if (intentName == 'orientation') {
        handleOrientationIntent(intent, session, callback);
    } else if (intentName == 'acceleration') {
        handleAccelerationIntent(intent, session, callback);
    } else if (intentName == 'temperature') {
        handleTemperatureIntent(intent, session, callback);
    } else if (intentName == 'pressure') {
        handlePressureIntent(intent, session, callback);
    } else if (intentName === 'AMAZON.HelpIntent') {
        sayHelloResponse(callback);
    } else if (intentName === 'AMAZON.StopIntent' || intentName === 'AMAZON.CancelIntent') {
        handleSessionEndRequest(callback);
    } else {
        throw new Error('Invalid intent');
    }
}

/**
 * Called when the user ends the session.
 * Is not called when the skill returns shouldEndSession=true.
 */
function onSessionEnded(sessionEndedRequest, session, callback) {
    console.log(`onSessionEnded requestId=${sessionEndedRequest.requestId}, sessionId=${session.sessionId}`);
    // Add cleanup logic here
}


// --------------- Main handler -----------------------

// Route the incoming request based on type (LaunchRequest, IntentRequest,
// etc.) The JSON body of the request is provided in the event parameter.
exports.handler = (event, context, callback) => {
    try {
        console.log(`event.session.application.applicationId=${event.session.application.applicationId}`);

        /**
         * Uncomment this if statement and populate with your skill's application ID to
         * prevent someone else from configuring a skill that sends requests to this function.
         * This ID is from the Alexa skill at developer.amazon.com
         */
        if (event.session.application.applicationId !== 'amzn1.ask.skill.0ccef166-fba3-42dd-8fe4-c7dd830100c2') {
             callback('Invalid Application ID');
        }

        if (event.session.new) {
            onSessionStarted({ requestId: event.request.requestId }, event.session);
        }
        console.log(`user: `);
        console.log(event.session.user.userId);
        console.log(`access token: `);
        console.log(event.session.user.accessToken);

        //if no token, return a LinkAccount card
        if (event.session.user.accessToken === undefined || null === event.session.user.accessToken)
        {
            console.log('Redirecting user to re-authenticate');
            callback (null,buildLinkAccountResponse());
        }

        if (event.request.type === 'LaunchRequest') {
            onLaunch(event.request,
                event.session,
                (sessionAttributes, speechletResponse) => {
                    callback(null, buildResponse(sessionAttributes, speechletResponse));
                });
        } else if (event.request.type === 'IntentRequest') {
            onIntent(event.request,
                event.session,
                (sessionAttributes, speechletResponse) => {
                    callback(null, buildResponse(sessionAttributes, speechletResponse));
                });
        } else if (event.request.type === 'SessionEndedRequest') {
            onSessionEnded(event.request, event.session);
        }
    } catch (err) {
        console.log(`err: ${err}`);
        callback(err);
    }
};


