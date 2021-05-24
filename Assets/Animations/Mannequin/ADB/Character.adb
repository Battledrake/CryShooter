<AnimDB FragDef="Animations/Mannequin/ADB/CharacterFragmentIds.xml" TagDef="Animations/Mannequin/ADB/CharacterTags.xml">
 <FragmentList>
  <Jump>
   <Fragment BlendOutDuration="0.2" Tags="Rifle">
    <AnimLayer>
     <Blend ExitTime="0" StartTime="0" Duration="0.2"/>
     <Animation name="unarmed_jump_loop" flags="Loop"/>
    </AnimLayer>
   </Fragment>
   <Fragment BlendOutDuration="0.2" Tags="">
    <AnimLayer>
     <Blend ExitTime="0" StartTime="0" Duration="0.00077056885"/>
     <Animation name="unarmed_jump_begin"/>
     <Blend ExitTime="0" StartTime="0" Duration="0.25077054"/>
     <Animation name="unarmed_jump_loop" flags="Loop"/>
    </AnimLayer>
   </Fragment>
  </Jump>
  <Locomotion>
   <Fragment BlendOutDuration="0.2" Tags="Rifle">
    <AnimLayer>
     <Blend ExitTime="0" StartTime="0" Duration="1.7881393e-07"/>
     <Animation name="BSpace_MoveStrafe_Rifle" flags="Loop"/>
    </AnimLayer>
   </Fragment>
   <Fragment BlendOutDuration="0.2" Tags="">
    <AnimLayer>
     <Blend ExitTime="0" StartTime="0" Duration="0"/>
     <Animation name="BSpace_MoveStrafe_Unarmed" flags="Loop"/>
    </AnimLayer>
   </Fragment>
  </Locomotion>
  <Fire>
   <Fragment BlendOutDuration="0.2" Tags="">
    <AnimLayer>
     <Blend ExitTime="0" StartTime="0" Duration="0"/>
     <Animation name="rifle_aim_fire" flags="Loop"/>
    </AnimLayer>
   </Fragment>
  </Fire>
 </FragmentList>
</AnimDB>
