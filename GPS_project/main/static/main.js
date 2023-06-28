//global variables
var bounding_area_created = false;
var polygon;
var data = "";
var circleIcon = L.icon({
    iconUrl: './static/circle.png',

    iconSize:     [6,6], // size of the icon
    iconAnchor:   [3,3], // point of the icon which will correspond to marker's location
});
var greemCircleIcon = L.icon({
    iconUrl: './static/circle2.png',

    iconSize:     [6,6], // size of the icon
    iconAnchor:   [3,3], // point of the icon which will correspond to marker's location
});
var marker = L.marker([40.63136, -8.65807], {icon: circleIcon});
var marker2 = L.marker([40.63136, -8.65807], {icon: greemCircleIcon});
var path =  [];
var polyline1;
var set_View = false;
// Function to make the AJAX request
function makeRequest() {
    $.ajax({
        url: "/get_data",
        method: "GET",
        dataType: "json",
        success: function(response) {
            // Check if response[lat] is equal to data[lat] and response[long] is equal to data[long  
            data = response; // Update the data variable
            if(response.Stored_Data){
                updateStoredPosition();
            }
            updateJavaScript(response); // Call the function to update JavaScript
        },
        error: function(xhr, status, error) {
            console.log("AJAX request failed:", error);
        }
    });
}

function initMap(response) {
    resetMap();

    // Create a new marker and polygon only if they don't exist
    if (!marker) {
        marker = L.marker([response.Latitude, response.Longitude], {icon: circleIcon}).addTo(map);
    }

    if (!polygon) {
        polygon = L.polygon([
            [response.Latitude + 0.0001, response.Longitude + 0.0001],
            [response.Latitude + 0.0001, response.Longitude - 0.0001],
            [response.Latitude - 0.0001, response.Longitude - 0.0001],
            [response.Latitude - 0.0001, response.Longitude + 0.0001]
        ]).addTo(map);
    }

    // Update the marker and polygon coordinates
    marker.setLatLng([response.Latitude, response.Longitude]);

    polygon.setLatLngs([
        [response.Latitude + 0.0001, response.Longitude + 0.0001],
        [response.Latitude + 0.0001, response.Longitude - 0.0001],
        [response.Latitude - 0.0001, response.Longitude - 0.0001],
        [response.Latitude - 0.0001, response.Longitude + 0.0001]
    ]);

    // Update map view
    //map.setView(marker.getLatLng(), 13);
    map.fitBounds(polygon.getBounds());
}
function resetMap() {
    if (marker) {
        map.removeLayer(marker);
    }
    //map.setView([40.63136, -8.65807], 13);
}
function checkArea(response) {
    var lat = response.Latitude;
    var lng = response.Longitude;

    var polygonCoordinates = polygon.getLatLngs()[0];
    var polygonBounds = polygon.getBounds();

    var isInsidePolygon = polygonBounds.contains([lat, lng]);

    if (isInsidePolygon) {
        document.getElementById('logMessage').className = 'badge badge-success';
        document.getElementById('logMessage').innerHTML = "Subject is inside the defined Area.";
    } else {
        document.getElementById('logMessage').className = 'badge badge-danger';
        document.getElementById('logMessage').innerHTML = "Subjest is outside the defined Area.";
    }
}

function updateJavaScript(response) {
    // Perform actions with the received data
    console.log("New data received:", response);
    console.log(response.Status);
    if(response.BoundingArea == true && bounding_area_created == false){
        initMap(response);
        bounding_area_created = true;
    }else if(response.Status == '65' && bounding_area_created == true){
            //remove old marker
        /*if (marker) {
            map.removeLayer(marker);
        }*/
        
        marker = L.marker([response.Latitude, response.Longitude], {icon: circleIcon}).addTo(map);
        if(!set_View){
            map.setView([response.Latitude, response.Longitude], 13);
            map.fitBounds(polygon.getBounds());
            set_View = true;
            
        }

        //path.push([response.Latitude, response.Longitude]);
        //polyline1 = L.polyline(path, { color: 'red' }).addTo(map);
                
        //update statistics fields
        document.getElementById('latitude').innerHTML = response.Latitude;
        document.getElementById('longitude').innerHTML = response.Longitude;
        document.getElementById('time').innerHTML = response.Time + 1;
        document.getElementById('speed').innerHTML = response.Speed;
        document.getElementById('course').innerHTML = response.Course;
        document.getElementById('status').className = 'badge badge-success'
        document.getElementById('status').innerHTML = "VALID"

        checkArea(response);
    }else if(response.Status == '86' && bounding_area_created == true){

        resetMap();
        set_View = false;
        document.getElementById('status').className = 'badge badge-danger'
        document.getElementById('status').innerHTML = "NOT VALID"
    }else if(response.Status == '86' && bounding_area_created == false){

        document.getElementById('status').className = 'badge badge-danger';
        document.getElementById('status').innerHTML = "NOT VALID";

    }

    // Update JavaScript variables or elements as needed
    // Example: document.getElementById('result').innerText = response;
}

function updateStoredPosition() {
    $.ajax({
        url: "/get_stored_data",
        method: "GET",
        dataType: "json",
        success: function(response) {
            //console.log("these are stored coords: ", response.Coordinates); // list, response.Coordinates[i] data array, response.coordinates[i][j] values
            console.log(response.Coordinates);
            for( var coords of response.Coordinates){
                marker2 = L.marker([coords[0], coords[1]], {icon: greemCircleIcon}).addTo(map);
            }
        },
        error: function(xhr, status, error) {
            console.log("AJAX request failed:", error);
        }
    });
}
// Make the initial request
makeRequest();

// Set the interval for periodic requests (every 5 seconds in this example)
setInterval(makeRequest, 250);



// Initialize the map
var map = L.map('map').setView([40.63136, -8.65807], 19);

// Add a tile layer (e.g., OpenStreetMap)
L.tileLayer('https://{s}.tile.openstreetmap.org/{z}/{x}/{y}.png', {
attribution: 'Map data &copy; <a href="https://www.openstreetmap.org/">OpenStreetMap</a> contributors',
maxZoom: 25,
}).addTo(map);

//var marker = L.marker([51.5, -0.09]).addTo(map);

//make map display at marker location
//map.setView(marker.getLatLng(), 13);

/*var circle = L.circle([40.63136, -8.65807], {
    color: 'red',
    fillColor: '#f03',
    fillOpacity: 0.5,
    radius: 500
}).addTo(map);*/

/*var polygon = L.polygon([
    [51.509, -0.08],
    [51.503, -0.06],
    [51.51, -0.047]
]).addTo(map);*/


window.onload = function() {
    $.ajax({
        url: "/get_data",
        method: "GET",
        dataType: "json",
        success: function(response) {
            // Check if response[lat] is equal to data[lat] and response[long] is equal to data[long  
            data = response; // Update the data variable

        },
        error: function(xhr, status, error) {
            console.log("AJAX request failed:", error);
        }
    });
}

