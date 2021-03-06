

# mDNS Setup Manual
##### prerequisites:
1. Accessible account at our flutter application.
2. SONOFF.
3. Hardware for the station.

With mDNS feature you are able to choose your own hostname for your station's Sonoff!

___

##### In order to setup a fully working station please follow these simple steps:
1. Add the station in the application.
2. Setup SONOFF (smart plug).
3. Setup your station.

___

### Add station in the application:
1. Login to the account who should own the station.

<img src="https://github.com/AseelAborokn/IOT-LabManager/blob/master/Project/src/imgs/home_page.png" width="250" hight="250">

2. Click on the `drawer` button on the top left corner of the `homepage` and select `My Stations` option.

<img src="https://github.com/AseelAborokn/IOT-LabManager/blob/master/Project/src/imgs/drawer.png" width="250" hight="250">

3. Click on `+` icon on the top right corner.

<img src="https://github.com/AseelAborokn/IOT-LabManager/blob/master/Project/src/imgs/my_stations.png" width="250" hight="250">

5. Fill all the fields and click on the `green button`

<img src="https://github.com/AseelAborokn/IOT-LabManager/blob/master/Project/src/imgs/CreateStation.png" width="250" hight="250">

___

### Setup SONOFF (smart plug):

1. Upload SONOFF code.
2. Hold the button of the SONOFF for until `SwithConfigurationsManager_AP` WiFi appears.
3. Connect to `SwithConfigurationsManager_AP` and click `WiFi Configure` in the page that pops up. If this page does not pop up automatically, in your browser navigate to port 192.168.4.1 and you should see the page.
4. Select the desired WiFi.
5. Insert the password to it.
6. Fill the desired `Hostname`.

<img src="https://github.com/AseelAborokn/IOT-LabManager/blob/master/Project/src/imgs/mDNS_ConfigureSonoff.jpeg" width="250" hight="250">

___

### Setup Station:

You will need to know your `Station Name` as it is in the app and the `Hostname` of your SONOFF which you have configured in the previous step.

After the receiving the info - 
1. Upload Station code.
2. Connect to `StationConfigurationManager_AP` and click `WiFi Configure` in the page that pops up. If this page does not pop up automatically, in your browser navigate to port 192.168.4.1 and you should see the page.
3. Select the desired WiFi, this should be the same WiFi you have set for your SONOFF.
4. Insert the password to it.
5. Fill the desired `Station's ID` as it is seen in the app.
6. Fill the desired `Sonoff Host Name` as you entered it in the previous step.

<img src="https://github.com/AseelAborokn/IOT-LabManager/blob/master/Project/src/imgs/mDNS_ConfigureStation.jpeg" width="250" hight="250">

Now you can use your station freely.
