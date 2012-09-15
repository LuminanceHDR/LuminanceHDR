#!/bin/bash
#
#./deploy-cli.sh
#
hdiutil create -format UDBZ -quiet -srcfolder luminance-hdr.app luminance-hdr.dmg
