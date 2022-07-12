""" List of Met Office Sites within a valid Unitary Authority Area """

# {
# 'elevation': '117.0',
# 'id': '3796',
# 'latitude': '51.133',
# 'longitude': '1.348',
# 'name': 'Langdon Bay',
# 'region': 'se',
# 'unitaryAuthArea': 'Kent'
# }

import json
import requests
import sys

# 1st arg - your key
mykey = sys.argv[1]
# 2nd arg - valid Unitary Authority Area
unityAuthArea = sys.argv[2]

# Get data from API
response = requests.get('http://datapoint.metoffice.gov.uk/public/data/val/wxfcs/all/json/sitelist?key=' + mykey)
data = response.json()

# Ensure all locations have a unitary area
filtered = [area for area in data['Locations']['Location'] if 'unitaryAuthArea' in area]

# List of unitary areas
arealist = []
for area in filtered:
    if area['unitaryAuthArea'] not in arealist:
        arealist.append(area['unitaryAuthArea'])
arealist.sort()

# List locations
sitelist = [site for site in filtered if site['unitaryAuthArea'] == unityAuthArea]
for site in sitelist:
    print(site['id'], site['name'])
