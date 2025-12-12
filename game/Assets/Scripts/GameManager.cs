using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using TMPro;
using UnityEditor.Rendering;

public class GameManager : MonoBehaviour
{
  public enum Scene {
    None,
    Ready,
    Play,
    Finish,
  }
  Scene currentScene = Scene.None;
  [SerializeField] Target targetPrefab;
  [SerializeField] Vector3 startTargetPos = new Vector3(0, 0, 0);
  [SerializeField] Vector2 range1 = new Vector2(-4, 3.5f);
  [SerializeField] Vector2 range2 = new Vector2(4, 1.5f);
  [SerializeField] float zPivot = 5;
  [SerializeField] float zRange = 1;
  [SerializeField] int countdownUntil = 5;
  [SerializeField] Intro intro;
  [SerializeField] TimeValue timerText;
  [SerializeField] GameObject result;
  [SerializeField] TimeValue resultTime;

  const int areaSize = 6;
  const int divides = 2;
  const string strongColor = "#fc8420";
  string introText = $"<color={strongColor}>A</color>re you <color={strongColor}>R</color>eady?";
  int[] areas = new int[areaSize];
  const int rows = divides, cols = areaSize / divides;
  float areaWidth, areaHeight;
  int count = 0;
  float countdownTime = 0;
  float countByStart = 0;
  float timer = 0;
  bool once = true;
  Target target;
  AudioSource audioSource;

  void Swap<T>(ref T a, ref T b) {
    var tmp = a;
    a = b;
    b = tmp;
  }

  T[] ShuffleAry<T>(T[] ary) {
    var length = areaSize;
    T[] shuffled = new T[length];

    // deep copy
    for (int i = 0; i < areaSize; i++) { shuffled[i] = ary[i]; }

    // shuffling
    for (int i = areaSize-1; i > 0; i--) {
      int j = Random.Range(0, areaSize);
      Swap<T>(ref shuffled[i], ref shuffled[j]);
    }
    return shuffled;
  }

  Target GenerateTarget(Target prefab) {
    var a = areas[count];
    int rowNum = (int)(a / (areaSize / 2));
    int colNum = (int)(a % (areaSize / 2));
    float z = Random.Range(-zRange, zRange) + zPivot;
    Vector3 pos = new Vector3(areaWidth*colNum + range1.x, range1.y - areaHeight*rowNum, z);
    Vector3 diffPos = new Vector3(Random.Range(0, areaWidth), -Random.Range(0, areaHeight), 0);

    var target = Instantiate(prefab);  
    target.transform.position = pos + diffPos;
    return target;
  }

  // Start is called before the first frame update
  void Start() {
    areaWidth = Mathf.Abs(range2.x - range1.x) / cols;
    areaHeight = Mathf.Abs(range2.y - range1.y) / rows;
    
    Cursor.visible = false;

    audioSource = GetComponent<AudioSource>();

    target = Instantiate(targetPrefab);  
    target.transform.position = startTargetPos;
  }

  // Update is called once per frame
  void Update() {
    if (Input.GetKeyDown(KeyCode.Escape)){
      Application.Quit();
    }

    switch(currentScene) {
      case Scene.None:
        if (target.clicked) { 
          currentScene = Scene.Ready;
          audioSource.PlayOneShot(target.hitSound);
          target.clicked = false;
        }
        count = 0;
        timer = 0;
        once = true;

        areas = ShuffleAry<int>(new int[6] { 0, 1, 2, 3, 4, 5 });
        countdownTime = countdownUntil;
        intro.SetText(introText);
        intro.gameObject.SetActive(true);
        countByStart = Random.Range(0, 1);

        result.gameObject.SetActive(false);
        resultTime.Reset();
        timerText.Reset();
        break;
      
      case Scene.Ready:
        countdownTime -= Time.deltaTime;

        if (countdownTime <= -1 + countByStart){
          intro.gameObject.SetActive(false);
          audioSource.PlayOneShot(audioSource.clip);
          currentScene = Scene.Play; 
        }
        else if (countdownTime <= 2) {
          intro.SetText("Stand by...");
        }
        break;
      
      case Scene.Play:
        timer += Time.deltaTime;
        timerText.time = timer;

        if (once) {
          target = GenerateTarget(targetPrefab);
          once = false;
        }
        if (target.clicked) {
          count++;
          audioSource.PlayOneShot(target.hitSound);
          target.clicked = false;

          if (count >= areaSize) {
            currentScene = Scene.Finish;
          } else {
            target = GenerateTarget(targetPrefab);
          }
        }
        break;

      case Scene.Finish:
        if (Input.GetKeyDown(KeyCode.R)){
          currentScene = Scene.None;
          target = Instantiate(targetPrefab);  
          target.transform.position = startTargetPos;
        }
        result.gameObject.SetActive(true);
        resultTime.time = timer;
        break;
    }
  }
}
