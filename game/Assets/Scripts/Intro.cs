using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class Intro : MonoBehaviour
{
  GameObject background;
  GameObject introText;
  TextMeshProUGUI introTextMesh;
  TextAnimator textAnimator;

  public void SetText(string newtext) {
    introTextMesh.text = newtext;
  }

  public void StartAnimation() {
    textAnimator.Run();
  }
  public void ReStartAnimation() {
    textAnimator.Restart();
  }
  // Start is called before the first frame update
  void Start() {
    background = transform.GetChild(0).gameObject;
    introText = transform.GetChild(1).gameObject;
    introTextMesh = introText.GetComponent<TextMeshProUGUI>();
    textAnimator = introText.GetComponent<TextAnimator>();
  }

  // Update is called once per frame
  void Update() {
    
  }
}
