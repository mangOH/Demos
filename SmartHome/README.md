Amazon Alexa Smart Home Demo
============================

This demonstration allows the user to query data from AirVantage using an Amazon Alexa.  The demo
is intended to stimulate ideas about what is possible when combining sensor data published to the
cloud and voice-based interaction.

Requirements
------------
* Amazon Echo or Dot
* mangOH Red
* Amazon Web Services account
* Amazon Developer Account
* WiFi hotspot
* Apple or Android smartphone or tablet. NOTE: Problems were encountered while using the Android
  app.


Steps To Enable the Demo
------------------------

**NOTE: These steps are a bit rough.  If you encounter any problems please post on the mangOH
forum**

1. Register and login to [developer.amazon.com](https://developer.amazon.com)
1. In the Amazon developer portal, click the "Alexa" tab and then click "Add a New Skill"
1. Choose "Custom Interaction Model" (not "Smart Home Skill API")
1. Choose a name for the skill and an invocation name.  The invocation name will be spoken to
   activate the skill, so choose something that is brief, but unique.
1. In the "Interaction Model" step, use the provided files `alexa_skill-schema.json,
   `alexa_skill-slot_types.txt` and `alexa_skill-sample_utterances.txt` to fill in the fields for
   "Intent Schema", "Custom Slot Types" and "Sample Utterances" respectively.
1. Register for an Amazon AWS account and login to the account.
1. Locate the "Lambda" service within AWS
1. Change your AWS region in the top right corner to N. Virginia
1. Create a new Lambda function using the Node.js 6.10 Blank Function template.
1. Fill in the content of the function using the code in file `amazon_lambda_service.js`
1. Under the advanced settings, set the timeout to 10s.
1. Copy the ARN from the top right of the AWS Lambda screen
1. Paste the ARN into the alexa developer endpoint box
1. Click yes for account linking and specify the url: https://eu.airvantage.net/api/oauth/authorize
1. Login to airvantage and click develop -> api clients
1. Add a new connector and paste the redirect url from developer.amazon.com
1. authorization grant type should be set to "implicit grant"
1. TODO: enable the skill in the app?
