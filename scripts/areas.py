"""List of valid Met Office Unitary Authority Areas"""

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

# Get data from API
response = requests.get('http://datapoint.metoffice.gov.uk/public/data/val/wxfcs/all/json/sitelist?key=' + mykey)
data = response.json()

# Ensure all locations have a unitary area
filtered = [area for area in data['Locations']['Location'] if 'unitaryAuthArea' in area]

area_list = []
for area in filtered:
    if area['unitaryAuthArea'] not in area_list:
        area_list.append(area['unitaryAuthArea'])

area_list.sort()
for area in area_list:
    print(area)
