@ECHO OFF

Echo Sending Webhook to Discord server

curl -i -H "Content-Type: application/json" -X POST -d "{ \"content\": \" %MESSAGE% \" }" "<<stripped>>"

Echo Message send to Discord server

exit /B 1