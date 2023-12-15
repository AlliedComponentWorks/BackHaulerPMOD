Module Module1

'This module holds top level definitions not specific to specific functionality.
'As functionality becomes more specific, more specific modules could be created.

'---------------------------------------------------------------------------------------
'Public variables
'---------------------------------------------------------------------------------------

'Keep track of the present configuration so things other than the direct control can edit it
Public PresentConfiguration As Byte = PMOD_CONFIGURATION_IO_ONLY

'---------------------------------------------------------------------------------------
'Kernel functions we use
'---------------------------------------------------------------------------------------

Declare Sub Sleep Lib "kernel32" (ByVal dwMilliseconds As UInteger)

'---------------------------------------------------------------------------------------
'Public functions  
'---------------------------------------------------------------------------------------

'Make the connector benign for transition between functions
Public Sub ConfigurePassive()
 'Return the interface to the most benign mode, I/O only with no configured pins
 PresentConfiguration = PMOD_CONFIGURATION_IO_ONLY
 SetPresentConfiguration()
 DisplayPresentConfiguration()
End Sub

'Set present configuration, including restoring drive states shown
Public Sub SetPresentConfiguration()

 'Set all push-pulls to false
 Form1.CheckBox_PP_PIN1.Checked = False
 Form1.CheckBox_PP_PIN2.Checked = False
 Form1.CheckBox_PP_PIN3.Checked = False
 Form1.CheckBox_PP_PIN4.Checked = False
 Form1.CheckBox_PP_PIN7.Checked = False
 Form1.CheckBox_PP_PIN8.Checked = False
 Form1.CheckBox_PP_PIN9.Checked = False
 Form1.CheckBox_PP_PIN10.Checked = False
 'Allow any such actual message to finish
 'These events should not really happen as long as the PP boxes respond to click and not checkchanged
 Application.DoEvents()
 Sleep(100)

 'Write the new configuration
 BHPMOD_SetConfiguration(PresentConfiguration)

End Sub

'Display the present configuration in the user status
'This also updates other user status controls for each associated condition
Public Sub DisplayPresentConfiguration()

 'Resume next because errors will happen here when the display is not yet fully formed on startup
 On Error Resume Next

 'Update the configuration LED
 Select Case PresentConfiguration
  Case PMOD_CONFIGURATION_IO_ONLY
   Form1.LED_Configuration.Text = "IO ONLY"
  Case PMOD_CONFIGURATION_SPI
   Form1.LED_Configuration.Text = "SPI"
  Case PMOD_CONFIGURATION_I2C
   Form1.LED_Configuration.Text = "I2C"
  Case PMOD_CONFIGURATION_SERIAL
   Form1.LED_Configuration.Text = "SERIAL"
 End Select

 'Update the display based on the user selected state and the configuration
 'This checkbox may be updated by something external, which if changed also gets us here
 If (Not Form1.CheckBox_Poll_Pin_State.Checked) Then
  Form1.LED_PIN1.Enabled = False
  Form1.LED_PIN2.Enabled = False
  Form1.LED_PIN3.Enabled = False
  Form1.LED_PIN4.Enabled = False
  Form1.LED_PIN7.Enabled = False
  Form1.LED_PIN8.Enabled = False
  Form1.LED_PIN9.Enabled = False
  Form1.LED_PIN10.Enabled = False
  Form1.CheckBox_PP_PIN1.Enabled = False
  Form1.CheckBox_PP_PIN2.Enabled = False
  Form1.CheckBox_PP_PIN3.Enabled = False
  Form1.CheckBox_PP_PIN4.Enabled = False
  Form1.CheckBox_PP_PIN7.Enabled = False
  Form1.CheckBox_PP_PIN8.Enabled = False
  Form1.CheckBox_PP_PIN9.Enabled = False
  Form1.CheckBox_PP_PIN10.Enabled = False
  Form1.LED_PIN1.BackColor = Color.Black
  Form1.LED_PIN2.BackColor = Color.Black
  Form1.LED_PIN3.BackColor = Color.Black
  Form1.LED_PIN4.BackColor = Color.Black
  Form1.LED_PIN7.BackColor = Color.Black
  Form1.LED_PIN8.BackColor = Color.Black
  Form1.LED_PIN9.BackColor = Color.Black
  Form1.LED_PIN10.BackColor = Color.Black
 Else
  Form1.LED_PIN1.Enabled = True
  Form1.LED_PIN2.Enabled = True
  Form1.LED_PIN3.Enabled = True
  Form1.LED_PIN4.Enabled = True
  Form1.LED_PIN7.Enabled = True
  Form1.LED_PIN8.Enabled = True
  Form1.LED_PIN9.Enabled = True
  Form1.LED_PIN10.Enabled = True
  Form1.CheckBox_PP_PIN1.Enabled = True
  Form1.CheckBox_PP_PIN2.Enabled = True
  Form1.CheckBox_PP_PIN3.Enabled = True
  Form1.CheckBox_PP_PIN4.Enabled = True
  Form1.CheckBox_PP_PIN7.Enabled = True
  Form1.CheckBox_PP_PIN8.Enabled = True
  Form1.CheckBox_PP_PIN9.Enabled = True
  Form1.CheckBox_PP_PIN10.Enabled = True
  Form1.LED_PIN1.BackColor = LED_COLOR_STATUS_OFF
  Form1.LED_PIN2.BackColor = LED_COLOR_STATUS_OFF
  Form1.LED_PIN3.BackColor = LED_COLOR_STATUS_OFF
  Form1.LED_PIN4.BackColor = LED_COLOR_STATUS_OFF
  Form1.LED_PIN7.BackColor = LED_COLOR_STATUS_OFF
  Form1.LED_PIN8.BackColor = LED_COLOR_STATUS_OFF
  Form1.LED_PIN9.BackColor = LED_COLOR_STATUS_OFF
  Form1.LED_PIN10.BackColor = LED_COLOR_STATUS_OFF
  Select Case PresentConfiguration
   Case PMOD_CONFIGURATION_SPI
    Form1.CheckBox_PP_PIN1.Enabled = False
    Form1.LED_PIN1.BackColor = Color.Black
    Form1.CheckBox_PP_PIN2.Enabled = False
    Form1.LED_PIN2.BackColor = Color.Black
    Form1.CheckBox_PP_PIN3.Enabled = False
    Form1.LED_PIN3.BackColor = Color.Black
    Form1.CheckBox_PP_PIN4.Enabled = False
    Form1.LED_PIN4.BackColor = Color.Black
   Case PMOD_CONFIGURATION_I2C
    Form1.CheckBox_PP_PIN3.Enabled = False
    Form1.LED_PIN3.BackColor = Color.Black
    Form1.CheckBox_PP_PIN4.Enabled = False
    Form1.LED_PIN4.BackColor = Color.Black
    Form1.CheckBox_PP_PIN9.Enabled = False
    Form1.LED_PIN9.BackColor = Color.Black
    Form1.CheckBox_PP_PIN10.Enabled = False
    Form1.LED_PIN10.BackColor = Color.Black
   Case PMOD_CONFIGURATION_SERIAL
    Form1.CheckBox_PP_PIN2.Enabled = False
    Form1.LED_PIN2.BackColor = Color.Black
    Form1.CheckBox_PP_PIN3.Enabled = False
    Form1.LED_PIN3.BackColor = Color.Black
  End Select
 End If

 'Refresh everything
 Application.DoEvents()
 Sleep(100)

End Sub

'---------------------------------------------------------------------------------------
'End of module          
'---------------------------------------------------------------------------------------

End Module