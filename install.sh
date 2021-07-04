#!/bin/bash

# A function to compare hashes
matchingHash() {
  if [ $1 == $2 ]
  then
    true;
  else
    false;
  fi
}

# Check if the user is root
if [ "$EUID" -ne 0 ]
  then echo "Please run as root"
  exit 1
fi

# Check if the api token is supplied
if [ $# -eq 0 ]
  then
    echo "A client ID and api token must be supplied"
    echo "Usage: sudo ./install.sh [CLIENT ID] [API TOKEN]"
    exit 1
fi

# Stop existing service
echo "Stopping existing Bakup Agent..."
service bakupagent stop

# Download dependencies for SSL
echo "Acquiring SSL dependencies..."
apt-get -qq install openssl ca-certificates libcurl4-openssl-dev -y

# Create the directories needed for storing files and the binary
echo "Creating directories..."
mkdir -p /opt/bakupagent
mkdir -p /etc/opt/bakupagent

# Get the user's ID
echo "Obtaining user ID..."
if [ -z "$3" ]
then
  USER_NAME=$(logname)
  USER_ID=$(id -u "$USER_NAME")
  touch /etc/opt/bakupagent/USER_ID
else
  USER_ID=$3
fi
echo "$USER_ID" | tee /etc/opt/bakupagent/USER_ID > /dev/null

# Create the credentials file for the user to populate
echo "Populating the client ID..."
CLIENT_ID=$1
touch /etc/opt/bakupagent/CLIENT_ID
echo "$CLIENT_ID" | tee /etc/opt/bakupagent/CLIENT_ID > /dev/null
chown "$USER_NAME" /etc/opt/bakupagent/CLIENT_ID

# Create the credentials file for the user to populate
echo "Populating the API token..."
API_TOKEN=$2
touch /etc/opt/bakupagent/API_TOKEN
echo "$API_TOKEN" | tee /etc/opt/bakupagent/API_TOKEN > /dev/null
chown "$USER_NAME" /etc/opt/bakupagent/API_TOKEN

# Set permissions of main directory
echo "Setting permissions..."
chown "$USER_NAME" /etc/opt/bakupagent/

# Create the service file to manage the service
echo "Making service file for systemd..."
echo "[Unit]
Description=Bakup Agent
After=network.target
StartLimitIntervalSec=0
StartLimitBurst=5

[Service]
Type=simple
ExecStart=/opt/bakupagent/bakupagent
Restart=on-failure
RestartSec=5s
KillMode=process

[Install]
WantedBy=multi-user.target" | tee /etc/systemd/system/bakupagent.service > /dev/null

# Reload the daemons
systemctl daemon-reload

# Download the executable
echo "Downloading Bakup Agent..."
wget -q https://agent.bakup.io/agent/latest -O /opt/bakupagent/bakupagent
chmod +x /opt/bakupagent/bakupagent

# Check agent hash
REMOTE_AGENT_HASH=$(wget -qO- https://agent.bakup.io/agent/latest/hash)
LOCAL_AGENT_HASH=$(sha512sum /opt/bakupagent/bakupagent  | cut -d " " -f 1)
if ! matchingHash $REMOTE_AGENT_HASH $LOCAL_AGENT_HASH
then
  echo "AGENT HASH DOES NOT MATCH WITH REMOTE HASH, EXITING"
  # Report error to Bakup
  wget -O/dev/null -q "https://bakup.io/api/agent/v1/hash/failed?type=agent&client_id=$CLIENT_ID&api_token=$API_TOKEN"
  exit 2
fi

# Get rclone binary
echo "Collecting rclone binary..."
wget -q https://agent.bakup.io/rclone/latest -O /opt/bakupagent/rclone
chmod +x /opt/bakupagent/rclone

# Check agent hash
REMOTE_RCLONE_HASH=$(wget -qO- https://agent.bakup.io/rclone/latest/hash)
LOCAL_RCLONE_HASH=$(sha512sum /opt/bakupagent/rclone  | cut -d " " -f 1)
if ! matchingHash $REMOTE_RCLONE_HASH $LOCAL_RCLONE_HASH
then
  echo "RCLONE HASH DOES NOT MATCH WITH REMOTE HASH, EXITING"
  # Report error to Bakup
  wget -O/dev/null -q "https://bakup.io/api/agent/v1/hash/failed?type=rclone&client_id=$CLIENT_ID&api_token=$API_TOKEN"
  exit 3
fi

# Get the uninstall script
echo "Getting uninstall script..."
wget -q https://agent.bakup.io/script/uninstall/latest -O /opt/bakupagent/uninstall.sh
chmod +x /opt/bakupagent/uninstall.sh

# Check uninstall hash
REMOTE_UNINSTALL_HASH=$(wget -qO- https://agent.bakup.io/script/uninstall/latest/hash)
LOCAL_UNINSTALL_HASH=$(sha512sum /opt/bakupagent/uninstall.sh  | cut -d " " -f 1)
if ! matchingHash $REMOTE_UNINSTALL_HASH $LOCAL_UNINSTALL_HASH
then
  echo "UNINSTALL SCRIPT HASH DOES NOT MATCH WITH REMOTE HASH, EXITING"
  # Report error to Bakup
  wget -O/dev/null -q "https://bakup.io/api/agent/v1/hash/failed?type=uninstall&client_id=$CLIENT_ID&api_token=$API_TOKEN"
  exit 4
fi

# Set permissions of the /opt/bakupagent directory
chown -R $USER_NAME:root /opt/bakupagent

# Start the service
echo "Starting Bakup service..."
systemctl enable bakupagent
service bakupagent stop
service bakupagent start

echo "DONE."
exit 0
