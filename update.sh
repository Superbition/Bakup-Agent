#!/bin/bash

# Check if the user is root
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit 1
fi

echo "Reading existing Client ID..."
if ! CLIENT_ID=$(cat /etc/opt/bakupagent/CLIENT_ID)
  then
    echo "Client ID is missing, exiting"
    exit 1
fi

echo "Reading existing API Token..."
if ! API_TOKEN=$(cat /etc/opt/bakupagent/API_TOKEN)
  then
    echo "API Token is missing, exiting"
    exit 1
fi

echo "Reading existing User ID..."
if ! USER_ID=$(cat /etc/opt/bakupagent/USER_ID)
  then
    echo "USER_ID is missing, exiting"
    exit 1
fi

echo "Uninstalling current agent..."
/opt/bakupagent/uninstall.sh

echo "Installing the agent..."
wget -qO - https://agent.bakup.io/install/latest | sudo bash -s $CLIENT_ID $API_TOKEN $USER_ID

echo "Update completed"