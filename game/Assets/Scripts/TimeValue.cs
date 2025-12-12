using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;

public class TimeValue : MonoBehaviour
{
  public float time;
  TextMeshProUGUI timeValue;

  string ConvertTime(float _time) {
    int minute = (int)(_time / 60);
    int second = (int)(_time % 60);
    int milisecond = (int)(_time * 100 % 100);
    string minText = (minute < 10) ? "0" + minute.ToString() : minute.ToString();
    string secText = (second < 10) ? "0" + second.ToString() : second.ToString();
    string msecText = (milisecond < 10) ? "0" + milisecond.ToString() : milisecond.ToString();
    return minText + ":" + secText + ":" + msecText;
  }

  public void Reset() {
    time = 0;
  }
  
  // Start is called before the first frame update
  void Start() {
    timeValue = GetComponent<TextMeshProUGUI>();
  }

  // Update is called once per frame
  void Update() {
    timeValue.text = ConvertTime(time);
  }
}
