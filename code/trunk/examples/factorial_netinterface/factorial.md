<metadata>
  <boxdefault>
    <interface value="C4SNet" />
  </boxdefault>
</metadata>

<metadata>
  <net name="factorial" >
     <observer value="true" />
     <observer_interactive value="true" />
  </net>

  <box name="factorial/condif" >
     <observer value="true" />
     <observer_interactive value="true" />
     <observer_port value="6556" />
  </box>  

  <net name="factorial/compute" >
     <observer value="true" />
     <observer_interactive value="true" />
  </net>   
</metadata>

// Uncomment this for file observers
/*
<metadata>
  <net name="factorial" >
     <observer value="true" />
     <observer_file value="factorial.obs" />
  </net>


  <net name="factorial/compute" >
     <observer value="true" />
     <observer_file value="compute.obs" />
  </net>   
</metadata>
*/

