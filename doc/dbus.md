
# D-BUS Backend Connection

The server also has a experimental d-bus connection, which could be used to feed the server with data from various feeders.
The W3C Sever exposes the below methods and these methods could be used (as methodcall) to fill the server with data.
  ```
  <interface name='org.eclipse.kuksa.w3cbackend'>
     <method name='pushUnsignedInt'>
       <arg type='s' name='path' direction='in'/>
       <arg type='t' name='value' direction='in'/>
     </method>
     <method name='pushInt'>
       <arg type='s' name='path' direction='in'/>
       <arg type='x' name='value' direction='in'/>
     </method>
     <method name='pushDouble'>
       <arg type='s' name='path' direction='in'/>
       <arg type='d' name='value' direction='in'/>
     </method>
     <method name='pushBool'>
       <arg type='s' name='path' direction='in'/>
       <arg type='b' name='value' direction='in'/>
     </method>
     <method name='pushString'>
       <arg type='s' name='path' direction='in'/>
       <arg type='s' name='value' direction='in'/>
     </method>
   </interface>
   ```