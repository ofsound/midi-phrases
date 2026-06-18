<script>
  import StepNumberDragInput from "./StepNumberDragInput.svelte";
  import { interfaceAccent } from "./rowAccentTheme.js";

  let {
    value = 100,
    onValueChange = () => {},
  } = $props();
</script>

<StepNumberDragInput
  boxed
  compact
  accent={interfaceAccent}
  {value}
  min={50}
  max={100}
  resetValue={100}
  ariaLabel="UI scale percent"
  {onValueChange}
/>
