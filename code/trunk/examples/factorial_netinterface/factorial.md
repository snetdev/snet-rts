<metadata>
  <boxdefault>
    <interface>C4SNet</interface>
  </boxdefault>
</metadata>

<metadata>
  <net name="factorial" >
     <observer value="true" />
     <observer_type value="before" />  
     <observer_interactive value="false" />
  </net>

  <box name="factorial/condif" >
     <observer value="true" />
     <observer_interactive value="true" />
     <observer_port value="6556" />
  </box>  

  <net name="factorial/compute" >
     <observer value="true" />
     <observer_type value="before" />  
     <observer_interactive value="false" />
  </net>   
</metadata>

