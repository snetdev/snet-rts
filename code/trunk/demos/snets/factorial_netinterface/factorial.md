<?xml version="1.0"?>
<metadata xmlns="snet-home.org" >

  <!-- C4SNet is the default interface -->
  <interface name="C4SNet" default="true" />

  <!-- Observers at the both sides of the top network -->
  <net name="factorial" >
     <observer data="full" type="both" interactive="true" />
   </net>

  <!-- Observer observing tags at compute network -->
  <net name="factorial/compute" >
     <observer data="full" type="before" interactive="true" />
  </net>
</metadata>
 

