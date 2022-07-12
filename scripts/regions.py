"""List of valid Met Office region codes"""

import json
import requests
import sys

# {
# 'elevation': '117.0',
# 'id': '3796',
# 'latitude': '51.133',
# 'longitude': '1.348',
# 'name': 'Langdon Bay',
# 'region': 'se',
# 'unitaryAuthArea': 'Kent'
# }

# 1st arg - your key
mykey = sys.argv[1]

response = requests.get('http://datapoint.metoffice.gov.uk/public/data/val/wxfcs/all/json/sitelist?key=' + mykey)
data = response.json()

# Ensure all locations have a unitary area
filtered = [region for region in data['Locations']['Location'] if 'region' in region]

# List of unitary areas
regionlist = []
for region in filtered:
    if region['region'] not in regionlist:
        regionlist.append(region['region'])
regionlist.sort()
for region in regionlist:
    print(region)
